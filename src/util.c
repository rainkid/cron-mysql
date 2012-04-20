#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <stdarg.h>

#include "util.h"
#include "define.h"

//日志函数
void write_log(const char *fmt,  ...) {
	char msg[BUFSIZE];
	FILE * fp = NULL;
	struct stat buf;
	stat(LOG_FILE, &buf);
	if (buf.st_size > MAX_LOG_SIZE) {
		rename(LOG_FILE, BACK_LOG_FILE);
	}

	//时间
	struct tm *p;
	time_t timep;
	time(&timep);
	p=localtime(&timep);
	//参数处理
	va_list  va;
	va_start(va, fmt);
	vsprintf(msg, fmt, va);
	va_end(va);
	//日志写入
	fp = fopen(LOG_FILE , "ab+" );
	if (fp) {
		fprintf(fp,"%04d-%02d-%02d %02d:%02d:%02d %s\n",(1900+p->tm_year),( 1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, msg);
	}
	fclose(fp);
}

long my_mktime (struct tm *tm) {
  long res;
  int year;
  year = tm->tm_year - 70;
  res = YEAR * year + DAY * ((year + 1) / 4);
  res += month[tm->tm_mon];
  if (tm->tm_mon > 1 && ((year + 2) % 4))
    res -= DAY;
  res += DAY * (tm->tm_mday - 1);
  res += HOUR * tm->tm_hour;
  res += MINUTE * tm->tm_min;
  res += tm->tm_sec;
  return res;

}

void spr_strcpy(char **dest, char *src){
	int s_len = (strlen(src) + 1) * sizeof(char);
	*dest = (char *)calloc(1, s_len);
	strncpy(*dest, src, s_len);
}

void spr_strcpy_fmt(char **dest, const char *fmt, ...){
	char p[BUFSIZE];
	va_list va;
	va_start(va, fmt);
	vsprintf(p, fmt, va);
	va_end(va);
	fprintf(stderr, "%s", p);
}

/* 打印错误信息 */
void print_error(const char *fmt, ...) {
	char error[BUFSIZE];
	va_list va;
	va_start(va, fmt);
	vsprintf(error, fmt, va);
	va_end(va);
	fprintf(stderr, "%s\n", error);
}

/* 内存释放函数 */
void var_free(void *p) {
	/*if (p) */free(p);
	p = NULL;
}

/* 内存分配函数 */
void *struct_calloc(size_t size) {
	void *p = calloc(size, 1);
	return p;
}
