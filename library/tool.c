#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <curl/curl.h>
#include <curl/easy.h>
#include <time.h>
#include <errno.h>

#include "base64.h"
#include "send_mail.h"


/* 发送请求结构体 */
struct ResponseStruct {
	char *responsetext;
	size_t size;
};

/* 字符串截取函数 */
char* substr(const char*str, unsigned start, unsigned end) {
	unsigned n = end - start;
	static char stbuf[256];
	strncpy(stbuf, str + start, n);
	stbuf[n] = 0;
	return stbuf;
}

/* 获取当前时间戳 */
time_t GetNowTime() {
	time_t nowTime;
	time(&nowTime);
	return nowTime;
}

/* Curl回调处理函数 */
size_t Curl_Callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	struct ResponseStruct *mem = (struct ResponseStruct *) data;
	mem->responsetext = realloc(mem->responsetext, mem->size + realsize + 1);
	if (mem->responsetext == NULL) {
		exit(EXIT_FAILURE);
	}
	memcpy(&(mem->responsetext[mem->size]), ptr, realsize);
	mem->size += realsize;
	mem->responsetext[mem->size] = 0;
	return realsize;
}

void test_mail(){
    int r =0;
    struct st_char_arry to_addrs[1];
    	to_addrs[0].str_p="15257128383@139.com";
    struct st_char_arry att_files[2];
    	att_files[0].str_p="";
    	att_files[1].str_p="";
	struct st_mail_msg_ mail;
	init_mail_msg(&mail);
	//锟皆凤拷锟斤拷锟斤拷锟斤拷要锟斤拷权锟斤拷锟斤拷证
	mail.authorization=AUTH_SEND_MAIL;
	//smtp.163.com
	//ip or server
	mail.server="smtp.qq.com";
	mail.port=25;
	mail.auth_user="289712388@qq.com";
	mail.auth_passwd="Rainkid,.0.";
	mail.from="aaaaa";
	mail.from_subject="no-reply@zhanglihai.com";
	mail.to_address_ary=to_addrs;
	mail.to_addr_len=1;
	mail.content="锟斤拷锟斤拷锟脚碉拷锟斤拷锟斤拷锟斤拷锟角猴拷<b>锟斤拷锟侥碉拷锟斤拷锟斤拷</b>锟斤拷锟斤拷锟脚碉拷锟斤拷锟斤拷锟斤拷锟角猴拷<b>锟斤拷锟侥碉拷锟斤拷锟斤拷</b>锟斤拷锟斤拷锟脚碉拷锟斤拷锟斤拷锟斤拷锟角猴拷<b>锟斤拷锟侥碉拷锟斤拷锟斤拷</b>锟斤拷锟斤拷锟脚碉拷锟斤拷锟斤拷锟斤拷锟角猴拷<b>锟斤拷锟侥碉拷锟斤拷锟斤拷</b>锟斤拷锟斤拷<font color=red>锟脚碉拷锟斤拷锟斤拷</font>锟斤拷锟角猴拷<b>锟斤拷锟侥碉拷锟斤拷锟斤拷</b>";
	mail.subject="锟斤拷锟斤拷<>锟斤拷锟斤拷!!";
	mail.mail_style_html=HTML_STYLE_MAIL;
	mail.priority=3;
	mail.att_file_len=2;
	mail.att_file_ary=att_files;
	r = send_mail(&mail);
	printf("send mail [%d]\n",r);
}

/* 发送请求 */
void Curl_Request(char *query_url) {
	CURL *curl_handle = NULL;
	CURLcode response;
	char *url = strdup(query_url);
	struct ResponseStruct chunk;
	chunk.responsetext = malloc(1);
	chunk.size = 0;

	fprintf(stderr, "%s", query_url);
	/* curl 选项设置 */
	curl_handle = curl_easy_init();
	if (curl_handle != NULL) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		/* 回调设置 */
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, Curl_Callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &chunk);
		response = curl_easy_perform(curl_handle);
	}

	/* 请求响应处理 */
	if ((response == CURLE_OK) && chunk.responsetext && (strstr(chunk.responsetext, "__programe_run_succeed__") != 0)) {
		fprintf(stderr, "[success]\n");
	} else {
		fprintf(stderr, "[fail]\n");
	}

	if (chunk.responsetext) {
		free(chunk.responsetext);
	}
	curl_easy_cleanup(curl_handle);
	free(url);
}
