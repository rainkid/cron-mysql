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
#include <sys/stat.h>
#include <stdarg.h>

#include "tool.h"
#include "define.h"

/* 获取当前时间戳 */
time_t GetNowTime() {
	time_t nowTime;
	time(&nowTime);
	return nowTime;
}

/* 日志函数 */
void write_log(const char *fmt,  ...) {
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
	p=localtime(&timep);
	/* 参数处理 */
	va_list  va;
	va_start(va, fmt);
	vsprintf(msg, fmt, va);
	va_end(va);
	/* 日志写入 */
	fp = fopen(LOG_FILE , "ab+" );
	if (fp) {
		fprintf(fp,"%04d-%02d-%02d %02d:%02d:%02d %s\n",(1900+p->tm_year),( 1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, msg);
	}
	fclose(fp);
}

/* 时间转换函数 */
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

/* 字符拷贝 */
char * string_copy(char *dest, const char *src) {
	int s_len;
	s_len = strlen(src) + 1;
	dest = malloc(s_len);
	memcpy(dest, src, s_len);
	return dest;
}
