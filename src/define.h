/*
 * define.h
 *
 *  Created on: 2011-12-20
 *      Author: rainkid
 */

#ifndef DEFINE_H_
#define DEFINE_H_

/* mysql连接相接相关数据 */
typedef struct mysql_params{
	char host[1024];
	char username[1024];
	char passwd[1024];
	char dbname[1024];
	int port;
} MysqlParams;

/* 邮件相关数据 */
typedef struct mail_params{
	char server[1024];
	char user[1024];
	char passwd[1024];
	char to[1024];
	int port;
} MailParams;

/* 请求返回数据 */
struct RESPONSE {
	char *responsetext;
	size_t size;
};
#endif /* DEFINE_H_ */

