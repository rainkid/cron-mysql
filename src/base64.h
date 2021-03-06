#ifndef BASE64_H_
#define BASE64_H_
#include <stdio.h>
#include <malloc.h>

void base64_encoder(const char *input,size_t len,char** out_str);


//写时候将fout移动到该文件最后 开始追加
//fin是 读入的文件
void base64_encoder_file(FILE *fin,FILE *fout);


void base64_decoder(const char *input,size_t len,char** out_str);

#endif	//BASE64_H_
