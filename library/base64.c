#include <string.h>
#include "base64.h"

static const char cb64[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const char rstr[] = {
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  62,   0,   0,   0,  63,
	 52,  53,  54,  55,  56,  57,  58,  59,  60,  61,   0,   0,   0,   0,   0,   0,
	  0,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
	 15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,   0,   0,   0,   0,   0,
	  0,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
	 41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,   0,   0,   0,   0,   0};

//使用完后要free(out_str);
void base64_encoder(const char *input,size_t len,char** out_str)
{
	//simple new buf length.
	size_t new_buf_len = len+len/3;
		   new_buf_len+=new_buf_len/76+2;
	char tmp_out[new_buf_len];
	size_t i=0;
	size_t o=0;
	size_t remain=0;
	size_t p=0;
	while(i<len)
	{
	 	remain = len-i;
	 	if(o&&o%76==0)
	 	 	tmp_out[p++]='\n';
	 	switch(remain)
	 	{
	 		case 1:
	 				tmp_out[p++]=cb64[((input[i] >> 2) & 0x3f)];
	 				tmp_out[p++]=cb64[((input[i] << 4) & 0x30)];
	 				tmp_out[p++]='=';
	 				tmp_out[p++]='=';
	 				break;
	 		case 2:
	 				tmp_out[p++]=cb64[((input[i] >> 2) & 0x3f)];
	 				tmp_out[p++]=cb64[((input[i] << 4) & 0x30) + ((input[i + 1] >> 4) & 0x0f)];
	 				tmp_out[p++]=cb64[((input[i + 1] << 2) & 0x3c)];
	 				tmp_out[p++]='=';
	 				break;
	 	    default:
	 				tmp_out[p++]=cb64[((input[i] >> 2) & 0x3f)];
	 				tmp_out[p++]=cb64[ ((input[i] << 4) & 0x30) + ((input[i + 1] >> 4) & 0x0f) ];
	 				tmp_out[p++]=cb64[((input[i + 1] << 2) & 0x3c) + ((input[i + 2] >> 6) & 0x03)];
	 				tmp_out[p++]=cb64[(input[i + 2] & 0x3f)];
	 	}//end switch
	 	i+=3;
	 	o+=4;
	}
	tmp_out[p]='\0';
//	printf("in:[%s],b6:[%s]\n",input,tmp_out);
	*out_str=(char *)malloc(new_buf_len*sizeof(char));
	memset(*out_str,0x0,new_buf_len);
	strcpy(*out_str,tmp_out);
}//end fun..

//read file to b6.
void base64_encoder_file(FILE *fin,FILE *fout)
{
	size_t remain =0;
	size_t o=0;
	size_t i=0;
	char input[4];
	remain = fread(input,1,3,fin);
	while(remain>0)
	{
		if(o&&o%76==0)
			fprintf(fout,"\n");
		switch(remain)
		{

		case 1:
				putc(cb64[((input[i] >> 2) & 0x3f)],fout);
				putc(cb64[((input[i] << 4) & 0x30)],fout);
				fprintf(fout,"==");
				break;
		case 2:
			    putc(cb64[((input[i] >> 2) & 0x3f)],fout);
				putc(cb64[((input[i] << 4) & 0x30) + ((input[i + 1] >> 4) & 0x0f)],fout);
				putc(cb64[((input[i + 1] << 2) & 0x3c)],fout);
				putc('=',fout);
				break;
		default:
				putc(cb64[((input[i] >> 2) & 0x3f)],fout);
				putc(cb64[ ((input[i] << 4) & 0x30) + ((input[i + 1] >> 4) & 0x0f) ],fout);
				putc(cb64[((input[i + 1] << 2) & 0x3c) + ((input[i + 2] >> 6) & 0x03)],fout);
				putc(cb64[(input[i + 2] & 0x3f)],fout);
		} //end switch
		o+=4;
		remain=fread(input,1,3,fin);
	}//end while
}//end fun file..


void base64_decoder(const char *input,size_t len,char** out_str)
{
	size_t i=0;
	size_t new_buf_len=len;
	size_t p = 0;
	char tmp_out[new_buf_len];
	while(i<len)
	{
		while(i<len&&(input[i]=='\n'||input[i]=='\r'))
			 i++;
		if(i<len)
		{
			char b1= (char)((rstr[(int)input[i]] << 2 & 0xfc) +
					(rstr[(int)input[i + 1]] >> 4 & 0x03));
				 tmp_out[p++]=b1;
				if(input[i+2]!='=')
				{
				   char b2 = (char)((rstr[(int)input[i+1]]<<4&0xf0)+(rstr[(int)input[i+2]]>>2&0x0f));
					tmp_out[p++]=b2;
				}
				if(input[i+3]!='=')
				{
					char b3= (char)((rstr[(int)input[i+2]]<<6&0xc0)+rstr[(int)input[i+3]]);
				    tmp_out[p++]=b3;
				}
			i+=4;
		}//end if...

	}// end while..
	tmp_out[p]='\0';

//	printf("decoder in:[%s],out_str:[%s]\n",input,tmp_out);
	*out_str = (char*)malloc(new_buf_len*sizeof(char));
	memset(*out_str,0x0,new_buf_len);
	strcpy(*out_str,tmp_out);
}

