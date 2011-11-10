#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <curl/curl.h>
#include <curl/easy.h>

#include <curl/curl.h>
#include <curl/easy.h>

#include <errno.h>

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
	if ((response == CURLE_OK) && chunk.responsetext && (strcmp(substr(chunk.responsetext, 0, 24), "__programe_run_succeed__") == 0)) {
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
