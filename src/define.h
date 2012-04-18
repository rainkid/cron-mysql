#ifndef DEFINE_H_
#define DEFINE_H_

#define PIDFILE  "/tmp/task.pid"
#define VERSION  "1.0"
#define BUFSIZE  4096
#define LOG_FILE  "/tmp/task.log"
#define BACK_LOG_FILE  "/tmp/task.log.bak"
#define MAX_LOG_SIZE  (1 * 1024 * 1024)

#define SYNC_CONFIG_TIME (5000000 * 60)
#define SEND_MAIL_TIME (1 * 60)
#define TIME_UNIT 60
#define TASK_STEP   100000

//#define __debug__ 1

/* mysql连接相接相关数据 */
struct mysql_params{
	char *host;
	char *username;
	char *passwd;
	char *dbname;
	int port;
};

/* 邮件相关数据 */
struct mail_params{
	char *server;
	char *user;
	char *passwd;
	char *to;
	int port;
};

/* 全局参数 */
struct server_params{
	char *run_type;
	char *notice;
	char *task_file;
	int max_threads;
	int shutdown;
	int mail_count;
};

/* CURL请求返回数据 */
struct s_response {
	char *responsetext;
	size_t size;
};

//即时邮件
struct right_mail {
	struct right_mail *next;
	char *content;
};

/* 即时任务 */
struct right_task {
	struct right_task *next;
	struct task_item * item;
};

typedef struct mysql_params s_mysql_params;
typedef struct mail_params s_mail_params;
typedef struct right_mail s_right_mail;
typedef struct server_params s_server_params;
typedef struct right_task s_right_task;

#endif /* DEFINE_H_ */

