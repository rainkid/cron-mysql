#ifndef DEFINE_H_
#define DEFINE_H_

#define PIDFILE  "/tmp/task.pid"
#define VERSION  "1.0"
#define BUFSIZE  4096
#define LOG_FILE  "/tmp/task.log"
#define BACK_LOG_FILE  "/tmp/task.log.bak"
#define MAX_LOG_SIZE  (1 * 1024 * 1024)

#define TIME_UNIT 60
#define SYNC_CONFIG_TIME (1000000 * TIME_UNIT * 5)
#define SEND_MAIL_TIME (30 * TIME_UNIT)
#define TASK_STEP   100000 * 5

// mysql连接相接相关数据
struct mysql{
	char *host;
	char *username;
	char *passwd;
	char *dbname;
	int port;
};

// 邮件相关数据
struct mail{
	char *server;
	char *user;
	char *passwd;
	char *to;
	int port;
};

//全局参数
struct server{
	char *run_from;
	char *notice_mail;
	char *task_file;
	char *cmd_type;
	int max_threads;
	int shutdown;
	int mail_count;
	int daemon;
};

//即时任务
struct right_task {
	struct right_task *next;
	struct task_item * item;
};

//请求返回数据
struct curl_response {
	char *text;
	size_t size;
};

//即时邮件
struct right_mail {
	struct right_mail *next;
	char *content;
};

typedef struct curl_response st_curl_response;
typedef struct right_task st_right_task;
typedef struct mysql st_mysql;
typedef struct server st_server;
typedef struct mail st_mail;
typedef struct right_mail st_right_mail;

#endif /* DEFINE_H_ */
