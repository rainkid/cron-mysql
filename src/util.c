#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <stdarg.h>

#include "define.h"
#include "util.h"

/* 获取当前时间戳 */
time_t GetNowTime() {
	time_t nowTime;
	time(&nowTime);
	return nowTime;
}

/* 内存释放函数 */
void my_free(void * p) {
#ifdef __debug__
	printf("%s:%d:%s free(%lx)\n", __FILE__, __LINE__, __func__, (unsigned long)p);
#endif
	if (p) free(p);
}

/* 内存分配函数 */
void * my_malloc(size_t size) {
	void *p;
	p = malloc(size);
#ifdef __debug__
	printf("malloc size=0x%lu, p=%lx\n", (unsigned long) size, (unsigned long) p);
#endif
	return p;
}

/* 日志函数 */
void write_log(const char *fmt, ...) {
	char msg[BUFSIZE];
	FILE * fp = NULL;
	struct stat buf;
	stat(LOG_FILE, &buf);
	if (buf.st_size > MAX_LOG_SIZE) {
		rename(LOG_FILE, BACK_LOG_FILE);
	}

	/* 时间 */
	struct tm *p;
	time_t timep;
	time(&timep);
	p = localtime(&timep);
	/* 参数处理 */
	va_list va;
	va_start(va, fmt);
	vsprintf(msg, fmt, va);
	va_end(va);
	/* 日志写入 */
	fp = fopen(LOG_FILE, "ab+");
	if (fp) {
		fprintf(fp, "%04d-%02d-%02d %02d:%02d:%02d %s\n", (1900 + p->tm_year),
				(1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec,
				msg);
	}
	fclose(fp);
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

/* 时间转换函数 */
long my_mktime(struct tm *tm) {
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

/* 字符拷贝 */
char * string_copy(char *dest, char *src) {
	int s_len;
	s_len = strlen(src) + 1;
	dest = malloc(s_len);
	memset(dest, '\0', s_len);
	memcpy(dest, src, s_len);
	return dest;
}
