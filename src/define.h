/*
 * define.h
 *
 *  Created on: 2011-12-20
 *      Author: rainkid
 */

#ifndef DEFINE_H_
#define DEFINE_H_

#define PIDFILE  "task.pid"
#define VERSION  "1.0"
#define BUFSIZE  8096
#define LOG_FILE  "logs/task.log"
#define BACK_LOG_FILE  "logs/task.log.bak"
#define MAX_LOG_SIZE  (1024 * 1000)

#define SYNC_CONFIG_TIME (5000000 * 60)
#define SEND_MAIL_TIME (30 * 1000000 * 60)
#define TIME_UNIT 60
#define TASK_STEP   1000000
#define THREAD_MAX 1024
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
//全局参数
typedef struct global_params{
	char run_type[1024];
	char notice[1024];
	char task_file[1024];
} GlobalParams;

/* 请求返回数据 */
struct Response {
	char *responsetext;
	size_t size;
};
#endif /* DEFINE_H_ */

