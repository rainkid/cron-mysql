/*
 * tool.c
 * This file is part of <task> 
 *
 * Copyright (C) 2011 - raink.kid@gmail.com
 *
 * <task> is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * <task> is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <errno.h>

#include "base64.h"
#include "mail.h"

#include <curl/curl.h>
#include <curl/easy.h>
/*******************************************************************/
/* 字符串截取函数 */
char* substr(const char*str, unsigned start, unsigned end) {
	unsigned n = end - start;
	static char stbuf[256];
	strncpy(stbuf, str + start, n);
	stbuf[n] = 0;
	return stbuf;
}
/*******************************************************************/


/*******************************************************************/
/* 获取当前时间戳 */
time_t GetNowTime() {
	time_t nowTime;
	time(&nowTime);
	return nowTime;
}
/*******************************************************************/


/*******************************************************************/
/*发送通知邮件*/
int send_notice_mail(char *subject, char *content){
    int ret =0;

    struct st_char_arry to_addrs[0];
	//收件人列表
    	to_addrs[0].str_p="15257128383@139.com";
    struct st_char_arry att_files[0];
	//附件列表
  	att_files[0].str_p="";
	struct st_mail_msg_ mail;
	init_mail_msg(&mail);
	mail.authorization=AUTH_SEND_MAIL;
	//smtp.163.com
	//ip or server
	mail.server="smtp.163.com";
	mail.port=466;
	mail.auth_user="rainkid@163.com";
	mail.auth_passwd="raink.kid";
	mail.from="rainkid@163.com";
	mail.from_subject="no-replyrainkid@163.com";
	mail.to_address_ary=to_addrs;
	mail.to_addr_len=1;
	mail.subject = "aaaaa";
	mail.content = "aaaaaaaaaaaaa";
	mail.mail_style_html=HTML_STYLE_MAIL;
	mail.priority=3;
	mail.att_file_len=2;
	mail.att_file_ary=att_files;
	ret = send_mail(&mail);
	fprintf(stderr, "Has %d mails send.\n", ret);
	return ret;
}
/*******************************************************************/


/*******************************************************************/
//发送短信
int send_notice_sms(char *url, char *subject, char *content){
	int ret = 0;
	CURL *curl_handle = NULL;
	CURLcode response;
	char hash[BUFSIZ] = "fR7lP3nNlZYdYwnq08=";
	char *sms_url;

	sms_url = malloc(strlen(url) + 1);
	strncpy(sms_url, url, strlen(url));

	char *post_data;
	post_data = malloc(strlen(subject) + strlen(content) + 50);

	sprintf(post_data, "subject=%s&content=%s&hash=%s", subject, content, hash);
	curl_handle = curl_easy_init();
	if(curl_handle){
		curl_easy_setopt(curl_handle, CURLOPT_URL, sms_url);
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 5);
		curl_easy_setopt(curl_handle, CURLOPT_POST, 1);
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, post_data);
		response = curl_easy_perform(curl_handle);
	}

	if (response == CURLE_OK){
		fprintf(stderr, "%s\n\n", post_data);
		ret = 1;
	}
	free(sms_url);
	curl_easy_cleanup(curl_handle);
	curl_global_cleanup();
	return ret;
}
