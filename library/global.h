/*
 * global.h
 *
 *  Created on: 2011-11-19
 *      Author: rainkid
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

//全局变量
char g_run_type[BUFSIZ] = {0x00};

char g_mysql_host[BUFSIZ] = {0x00};
char g_mysql_username[BUFSIZ] = {0x00};
char g_mysql_passwd[BUFSIZ] = {0x00};
char g_mysql_dbname[BUFSIZ] = {0x00};
int g_mysql_port = 0;

char g_notice[BUFSIZ] = {0x00};

#endif /* GLOBAL_H_ */
