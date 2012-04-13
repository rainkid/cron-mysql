/*
 * define.h
 *
 *  Created on: 2011-12-20
 *      Author: rainkid
 */

#ifndef DEFINE_H_
#define DEFINE_H_

#define PIDFILE  "/tmp/task.pid"
#define VERSION  "1.0"
#define BUFSIZE  4096
#define LOG_FILE  "/tmp/task.log"
#define BACK_LOG_FILE  "/tmp/task.log.bak"
#define MAX_LOG_SIZE  (1 * 1024 * 1024)

#define SYNC_CONFIG_TIME (5000000 * 60)
#define SEND_MAIL_TIME (30 * 60)
#define TIME_UNIT 60
#define TASK_STEP   100000
/* mysql连接相接相关数据 */
typedef struct mysql_params{
	char *host;
	char *username;
	char *passwd;
	char *dbname;
	int port;
} s_mysql_params;

/* 邮件相关数据 */
typedef struct mail_params{
	char *server;
	char *user;
	char *passwd;
	char *to;
	int port;
} s_mail_params;

/* 全局参数 */
typedef struct server_params{
	char *run_type;
	char *notice;
	char *task_file;
	int max_threads;
	int shutdown;
	int mail_count;
} s_server_params;

/* CURL请求返回数据 */
struct s_response {
	char *responsetext;
	size_t size;
};

//即时邮件
struct s_right_mail {
	struct s_right_mail *next;
	char content[1024];
};

#endif /* DEFINE_H_ */

