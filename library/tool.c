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
#include "send_mail.h"

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
