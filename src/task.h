#ifndef TASK_H_
#define TASK_H_

#define malloc(s) my_malloc(s)
#define free(p) my_free(p)

/* 配置文件 */
char *g_config_file = NULL;
/* 任务节点 */
l_task_list *task_list = NULL;
/* mysql参数 */
s_mysql_params g_mysql_params;
/* 邮件参数 */
s_mail_params g_mail_params;
/* 全局变量 */
s_server_params server;

/* 任务锁 */
static pthread_mutex_t LOCK_task = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t LOCK_right_task = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t LOCK_right_mail = PTHREAD_MUTEX_INITIALIZER;
/* 信号量 */
static pthread_cond_t COND_right_task = PTHREAD_COND_INITIALIZER;
static pthread_cond_t COND_right_mail = PTHREAD_COND_INITIALIZER;
/* 即时任务 */
typedef struct right_task {
	struct right_task *next;
	s_task_item * item;
}s_right_task;

/* 即时任务链表 */
struct right_task *l_right_task = NULL;
struct right_mail *l_right_mail = NULL;

void usage();
bool task_mysql_connect(MYSQL *mysql_conn);
void task_mysql_close(MYSQL *mysql_conn);
void mail_worker();
void task_worker();
void load_worker();
void push_right_task(s_task_item *item);
size_t curl_callback(void *ptr, size_t size, size_t nmemb, void *data);
int send_notice_mail(char *subject, char *content);
void task_log(int task_id, int ret, char* msg);
void curl_request(s_task_item *item);
void shell_command(s_task_item *item);
void push_mail(s_task_item * item);
void load_file_tasks();
void load_mysql_tasks();
void free_global_params();
void free_mysql_params();
void free_mail_params();
void free_resource();
void init_global_params();
void init_mysql_params();
void init_mail_params();
void deal_task();
void create_threads();
void signal_master(const int signal);
void signal_worker(const int signal);
void create_child(void);
void create_pid_file(void);
void daemonize(void);
void config_testing();

#endif /* TASK_H_ */
