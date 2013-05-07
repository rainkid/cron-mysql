#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <assert.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/wait.h>
#include <math.h>
#include <sys/prctl.h>
#include <netinet/in.h> 
#include <sys/socket.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <mysql/mysql.h>

#include "define.h"
#include "util.h"
#include "list.h"
#include "config.h"
#include "base64.h"
#include "mail.h"

//全局变量
char *g_config_file = NULL;
// 任务节点
lt_task_list *task_list = NULL;
//mysql参数
st_mysql *g_mysql = NULL;
//邮件参数
st_mail *g_mail = NULL;
//全局变量
st_server *g_server = NULL;

//任务
static pthread_mutex_t LOCK_task = PTHREAD_MUTEX_INITIALIZER;
//即时任务队列
static pthread_mutex_t LOCK_right_task = PTHREAD_MUTEX_INITIALIZER;
//即时邮件队列
static pthread_mutex_t LOCK_right_mail = PTHREAD_MUTEX_INITIALIZER;
//共享curl
static pthread_mutex_t LOCK_share_handle = PTHREAD_MUTEX_INITIALIZER;
//信号量
static pthread_cond_t COND_right_task = PTHREAD_COND_INITIALIZER;
static pthread_cond_t COND_right_mail = PTHREAD_COND_INITIALIZER;

//即时任务链表
st_right_task *l_right_task = NULL;
st_right_mail *l_right_mail = NULL;

static CURLSH *share_handle = NULL;

void usage();
void task_mysql_connect(MYSQL *mysql_conn);
void task_mysql_close(MYSQL *mysql_conn);
void mail_worker();
void task_worker();
void load_worker();
void scan_task_list();
void push_right_task(st_task_item * item);
size_t curl_callback(void *ptr, size_t size, size_t nmemb, void *data);
int send_notice_mail(char *subject, char *content);
void task_log(int task_id, int ret, char* msg);
void curl_request(st_task_item *item);
void curl_share_handle(CURL * curl_handle);
void shell_command(st_task_item *item);
void push_mail(st_task_item * item);
void load_file_tasks();
void load_mysql_tasks();
void init_main();
void init_mysql();
void init_mail();
void free_main();
void free_mysql();
void free_mail();
void free_resource();
void deal_task();
void create_threads();
void sig_master(const int sig);
void sig_worker(const int sig);
void create_child(void);
void create_pid_file(void);
void daemonize(void);

/* 帮助信息 */
void usage() {
	printf("author raink.kid@gmail.com\n"
		"-h, --help     display this message then exit.\n"
		"-v, --version  display version information then exit.\n"
		"-c, --config   <path>  task config file path.\n"
		"-d, --daemon   run as a daemon.\n\n");
}

/* mysql连接 */
void task_mysql_connect(MYSQL *mysql_conn) {
	my_init();
	if (NULL == mysql_init(mysql_conn)) {
		write_log("Colud not init mysql.");
	}
	if (mysql_library_init(0, NULL, NULL)) {
		write_log("Could not initialize mysql library.");
	}
	if (!mysql_real_connect(mysql_conn,
			g_mysql->host,
			g_mysql->username,
			g_mysql->passwd,
			g_mysql->dbname,
			g_mysql->port,
			NULL,
			0)) {
		write_log("Mysql connect error : .", mysql_error(mysql_conn));
	}
}

/* 关闭mysql */
void task_mysql_close(MYSQL *mysql_conn) {
	mysql_close(mysql_conn);
	mysql_conn = NULL;
	mysql_library_end();
}

/* 邮件队列 */
void mail_worker() {
	pthread_detach(pthread_self());
	st_right_mail *right_mail;
	char subject[BUFSIZE] = { 0x00 };
	char content[BUFSIZE] = { 0x00 };

	while (!g_server->shutdown) {
		if (NULL != l_right_mail) {
			pthread_mutex_lock(&LOCK_right_mail);
			right_mail = l_right_mail;
			l_right_mail = right_mail->next;
			g_server->mail_count--;
			pthread_mutex_unlock(&LOCK_right_mail);
			sprintf(subject, "Taskserver with some error!");
			sprintf(content, "Error with [%s] \n", right_mail->content);
			send_notice_mail(subject, content);
			free_var(right_mail->content);
			free_var(right_mail);
		}
		sleep(SEND_MAIL_TIME);
	}
}

/*发送通知邮件*/
int send_notice_mail(char *subject, char *content) {
	int ret = 0;
	struct st_char_arry to_addrs[1];
	//收件人列表
	to_addrs[0].str_p = g_mail->to;
	struct st_char_arry att_files[0];
	//附件列表
	att_files[0].str_p = "";
	st_mail_msg mail;
	init_mail_msg(&mail);
	spr_strcpy(&mail.subject, subject);
	spr_strcpy(&mail.content, content);
	mail.authorization = AUTH_SEND_MAIL;
	mail.server = g_mail->server;
	mail.port = g_mail->port;
	mail.auth_user = g_mail->user;
	mail.auth_passwd = g_mail->passwd;
	mail.from = g_mail->user;
	mail.from_subject = "TaskError@163.com";
	mail.to_address_ary = to_addrs;
	mail.to_addr_len = 1;
	mail.mail_style_html = HTML_STYLE_MAIL;
	mail.priority = 3;
	mail.att_file_len = 2;
	mail.att_file_ary = att_files;
	ret = send_mail(&mail);
	write_log("Send an mail with retval : %d.", ret);
	return ret;
}

/* 任务处理线程 */
void task_worker() {
	pthread_detach(pthread_self());
	while (!g_server->shutdown) {
		pthread_mutex_lock(&LOCK_task);
		scan_task_list();
		pthread_mutex_unlock(&LOCK_task);
		usleep(TASK_STEP);
	}
}

void scan_task_list(){
	st_task_item *task_item;
	if (NULL != task_list && task_list->count > 0) {
		while (NULL != (task_item = (st_task_item *)task_list->head)) {
			// 大于当前时间跳出
			time_t now_time;
			time(&now_time);
			if (now_time < task_item->next_time) {
				break;
			}
			push_right_task(task_item);
			(task_item->run_times)++;
			if (task_item->next) {
				task_list->head = task_item->next;
				task_item->next->prev = NULL;
				(task_list->count)--;
				if (task_item == task_list->tail) {
					task_list->tail = task_item->prev;
				}
				if (0 == task_item->times || task_item->run_times < task_item->times) {
					task_item->prev = NULL;
					task_item->next = NULL;
					task_item->next_time = now_time + task_item->step;
					update_task(task_item, task_list);
				} else {
					delete_item(task_item, task_list);
					task_item = NULL;
				}
			} else {
				// 已经达到执行次数,抛出队列
				if (task_item->times > 0 && task_item->run_times > task_item->times) {
					delete_item(task_item, task_list);
					task_item = NULL;
					free_task(task_list);
				} else {
					task_item->next_time = now_time + task_item->step;
				}
			}
		}
	}
}

/* 推送到任务队列 */
void push_right_task(st_task_item * item){
	st_right_task *right_item;
	right_item = calloc(1, sizeof(st_right_task));
	right_item->item = calloc(1, sizeof(st_task_item));
	copy_item(right_item->item, item);
	pthread_mutex_lock(&LOCK_right_task);
	right_item->next = l_right_task;
	l_right_task = right_item;
	pthread_mutex_unlock(&LOCK_right_task);
	pthread_cond_broadcast(&COND_right_task);
}

/* http请求 */
void curl_request(st_task_item *item) {
	int ret = 0;
	CURL *curl_handle;

	st_curl_response reponse;

	CURLcode retval;
	curl_handle = curl_easy_init();
	curl_share_handle(curl_handle);

	st_task_item * task_item;
	task_item = calloc(1, sizeof(st_task_item));
	copy_item(task_item, item);

	if (curl_handle != NULL) {
		curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL,	1L);
		curl_easy_setopt(curl_handle, CURLOPT_URL, task_item->command);
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, task_item->timeout * TIME_UNIT);
		// 回调设置
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curl_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &reponse);

		retval = curl_easy_perform(curl_handle);
	}
	// 请求响应处理
	if ((retval == CURLE_OK) && reponse.text &&
		(strstr(reponse.text, "__programe_run_succeed__") != 0)) {
		write_log("%s...success", task_item->command);
		ret = 1;
	} else {
		write_log("%s...fails", task_item->command);
		push_mail(task_item);
	}
	//记录日志
	if (strcmp(g_server->run_from, "mysql") == 0) {
		if (NULL != reponse.text) {
			task_log(item->task_id, ret, reponse.text);
		}
	}

	if (NULL != reponse.text) {
		free_var(reponse.text);
	}
	free_item(task_item);
	curl_easy_cleanup(curl_handle);
}

void curl_share_handle(CURL * curl_handle) {
	if (!share_handle) {
		if (0 != pthread_mutex_lock(&LOCK_share_handle)) {
			return;
		}
		if (!share_handle) {
			share_handle = curl_share_init();
			curl_share_setopt(share_handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
		}
		pthread_mutex_unlock(&LOCK_share_handle);
	}
	curl_easy_setopt(curl_handle, CURLOPT_SHARE, share_handle);
	curl_easy_setopt(curl_handle, CURLOPT_DNS_CACHE_TIMEOUT, 60);
}

void shell_command(st_task_item *item) {
	FILE * fp;
	int ret = 0;
	char text[1024];

	st_task_item * task_item;
	task_item = calloc(1, sizeof(st_task_item));
	copy_item(task_item, item);

	fp = popen(item->command, "r");
	if(NULL != fgets(text, sizeof(text), fp)){
		if (text && strstr(text, "__programe_run_succeed__") != 0) {
			ret = 1;
			write_log("%s...success", task_item->command);
		} else {
			write_log("%s...failed", task_item->command);
			push_mail(task_item);
		}
	}
	// 记录日志
	if (strcmp(g_server->run_from, "mysql") == 0) {
		if (NULL != text) {
			task_log(task_item->task_id, ret, text);
		}
	}
	free_item(task_item);
	pclose(fp);
}

/* 推送到邮件队列 */
void push_mail(st_task_item * item) {
	st_right_mail *right_mail;
	if (g_server->mail_count < 6) {
		right_mail = calloc(1, sizeof(st_right_mail));
		spr_strcpy(&right_mail->content, item->command);
		pthread_mutex_lock(&LOCK_right_mail);
		right_mail->next = l_right_mail;
		l_right_mail = right_mail;
		g_server->mail_count++;
		pthread_mutex_unlock(&LOCK_right_mail);
	}
}

/* curl回调处理函数 */
size_t curl_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	st_curl_response *reponse = (st_curl_response *) data;
	reponse->text = NULL;
	reponse->size = 0;

	reponse->text = calloc(1, realsize + 1);
	if (NULL == reponse->text) {
		write_log("Response text malloc faild.");
		return;
	}
	memcpy(reponse->text, ptr, realsize);
	reponse->size += realsize;
	return reponse->size;
}

//记录日志
void task_log(int task_id, int ret, char* msg) {
	MYSQL mysql_conn;
	char sql[2048] = { 0x00 };
	char upsql[1024] = { 0x00 };

	struct tm *p;
	time_t timep;
	time(&timep);
	p = localtime(&timep);

	task_mysql_connect(&mysql_conn);
	if(mysql_errno(&mysql_conn) == 0) {
			//更新执行时间
			sprintf(upsql, "UPDATE mk_timeproc SET last_run_time='%04d-%02d-%02d %02d:%02d:%02d' WHERE id=%d",
					(1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday,
					p->tm_hour, p->tm_min, p->tm_sec, task_id);
			if (mysql_real_query(&mysql_conn, upsql, strlen(upsql)) != 0) {
				write_log("Could not update last_run_time.");
			}
			//添加日志
			sprintf(sql, "INSERT INTO mk_timeproc_log VALUES('', %d,%d,'%s' ,'%04d-%02d-%02d %02d:%02d:%02d')",
					task_id, ret, msg, (1900 + p->tm_year), (1 + p->tm_mon),
					p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
			if (mysql_real_query(&mysql_conn, sql, strlen(sql)) != 0) {
				write_log("Could insert task log into mysql.");
			}
	}
	task_mysql_close(&mysql_conn);
}

/* 同步配置线程 */
void load_worker() {
	pthread_detach(pthread_self());
	while (!g_server->shutdown) {
		// 加载任务到新建列表中
		if (strcmp(g_server->run_from, "file") == 0) {
			load_file_tasks();
		} else if (strcmp(g_server->run_from, "mysql") == 0) {
			load_mysql_tasks();
		}
		usleep(SYNC_CONFIG_TIME);
	}
}

// 文件配置计划任务加载
void load_file_tasks() {
	FILE *fp;
	fp = fopen(g_server->task_file, "r");
	if (NULL == fp) {
		write_log("Open config file faild.");
	}
	char line[BUFSIZE] = { 0x00 };

	pthread_mutex_lock(&LOCK_task);
	//初始化任务列表
	init_task(task_list);
	pthread_mutex_unlock(&LOCK_task);

	while (NULL != fgets(line, BUFSIZE, fp)) {
		// 忽略空行和＃号开头的行
		if ('\n' == line[0] || '#' == line[0]) {
			continue;
		}
		line[strlen(line) + 1] = '\0';
		struct tm stime, etime;

		// 创建并初始化一个新节点
		st_task_item *task_item;
		task_item = (st_task_item *) calloc(1, sizeof(st_task_item));

		task_item->next = NULL;
		task_item->prev = NULL;
		task_item->run_times = 0;

		// 按格式读取
		sscanf(line, "%04d-%02d-%02d %02d:%02d:%02d,%04d-%02d-%02d %02d:%02d:%02d,%d,%d,%[^\n]",
				&stime.tm_year, &stime.tm_mon, &stime.tm_mday,
				&stime.tm_hour, &stime.tm_min, &stime.tm_sec,
				&etime.tm_year, &etime.tm_mon, &etime.tm_mday,
				&etime.tm_hour, &etime.tm_min, &etime.tm_sec,
				&task_item->step, &task_item->times, task_item->command);

		stime.tm_year -= 1900;
		etime.tm_year -= 1900;
		stime.tm_mon -= 1;
		etime.tm_mon -= 1;

		task_item->step = task_item->step * TIME_UNIT;

		task_item->start_time = my_mktime(&stime);
		task_item->end_time = my_mktime(&etime);
		time_t now_time;
		time(&now_time);

		// 如果已经结束直接下一个
		if (task_item->end_time <= now_time || task_item->next_time > task_item->end_time) {
			free_var(task_item);
			continue;
		}

		// 计算下次运行点
		int step = ceil((now_time - task_item->start_time) / task_item->step);
		task_item->next_time = task_item->start_time + ((step + 1) * task_item->step);
		while (task_item->next_time <= now_time) {
			task_item->next_time += task_item->step;
		}
		pthread_mutex_lock(&LOCK_task);
		// 更新到任务链表
		update_task(task_item, task_list);
	}
	write_log("Load %d tasks form file.", task_list->count);
	fclose(fp);
}

void row_to_item(MYSQL_ROW mysql_row, st_task_item *task_item){
	struct tm stime, etime;
	int run_type;
	sscanf(mysql_row[1], "%04d-%02d-%02d %02d:%02d:%02d",
			&stime.tm_year,	&stime.tm_mon,
			&stime.tm_mday, &stime.tm_hour,
			&stime.tm_min, &stime.tm_sec);
	sscanf(mysql_row[2], "%04d-%02d-%02d %02d:%02d:%02d",
			&etime.tm_year,	&etime.tm_mon,
			&etime.tm_mday, &etime.tm_hour,
			&etime.tm_min, &etime.tm_sec);

	stime.tm_year -= 1900;
	etime.tm_year -= 1900;
	stime.tm_mon -= 1;
	etime.tm_mon -= 1;

	task_item->mail = false;
	task_item->next = NULL;
	task_item->prev = NULL;
	task_item->run_times = 0;
	task_item->times = 0;
	task_item->step = atoi(mysql_row[3]) * TIME_UNIT;
	task_item->task_id = atoi(mysql_row[0]);
	task_item->timeout = atoi(mysql_row[7]);
	task_item->start_time = my_mktime(&stime);
	task_item->end_time = my_mktime(&etime);
	spr_strcpy(&task_item->command, mysql_row[5]);
	run_type = atoi(mysql_row[4]);
	if (run_type == 1) {
		task_item->deal_func = (deal_func)curl_request;
	} else {
		task_item->deal_func = (deal_func)shell_command;
	}
}

void load_mysql_tasks() {
	MYSQL mysql_conn;
	MYSQL_RES *mysql_result;
	MYSQL_ROW mysql_row;
	char *query_sql = "SELECT * FROM mk_timeproc where isopen = 1";

	task_mysql_connect(&mysql_conn);

	if(mysql_errno(&mysql_conn) == 0) {
		// 查询结果
		if (mysql_real_query(&mysql_conn, query_sql, strlen(query_sql)) != 0) {
			write_log("Query result from mk_timeproc failed.");
		}
		// 获取结果集和条数
		mysql_result = mysql_use_result(&mysql_conn);

		if (mysql_result != NULL) {
			pthread_mutex_lock(&LOCK_task);
			//初始化任务列表
			init_task(task_list);
			pthread_mutex_unlock(&LOCK_task);
			// 取数据
			while (mysql_row = mysql_fetch_row(mysql_result)) {
				// 开始,结束时间
				time_t now_time;
				time(&now_time);

				st_task_item *task_item;
				task_item = (st_task_item *) calloc(1, sizeof(st_task_item));
				init_task_item(task_item);

				row_to_item(mysql_row, task_item);
				// 计算下次运行点
				int step = ceil((now_time - task_item->start_time) / task_item->step);
				task_item->next_time = task_item->start_time + ((step + 1)	* task_item->step);

				while (task_item->next_time <= now_time) {
					task_item->next_time += task_item->step;
				}
				// 如果已经结束直接下一个
				if (task_item->end_time <= now_time || task_item->next_time	> task_item->end_time) {
					free_item(task_item);
					continue;
				}
				// 更新到任务链表
				pthread_mutex_lock(&LOCK_task);
				update_task(task_item, task_list);
				pthread_mutex_unlock(&LOCK_task);
			}
		}
	}

	mysql_free_result(mysql_result);
	write_log("Load %d tasks from mysql.", task_list->count);

	task_mysql_close(&mysql_conn);
}

void get_config_string(char **dest, char *area, char *key){
	spr_strcpy(dest, c_get_string(area, key, g_config_file));
}

int get_config_int(int *dest, char * area, char * key){
	*dest = c_get_int(area, key, g_config_file);
}

void init_main() {
	get_config_string(&g_server->run_from, "main", "run_from");
	if (strcmp(g_server->run_from, "file") != 0 && strcmp(g_server->run_from, "mysql") != 0) {
		print_error("Error value with run_from : %s", g_server->run_from);
		exit(0);
	}

	get_config_string(&g_server->notice_mail, "main", "notice_mail");
	if (strcmp(g_server->notice_mail, "on") != 0 && strcmp(g_server->notice_mail, "off") != 0) {
		print_error("Error notice_mail value : %s",	g_server->notice_mail);
		exit(0);
	}

	get_config_int(&g_server->max_threads, "main", "max_threads");
	g_server->shutdown = 0;
	g_server->mail_count = 5;
}

void free_main(){
	free_var(g_server->run_from);
	free_var(g_server->task_file);
	free_var(g_server->notice_mail);
	free_var(g_server);
}

void init_mysql() {
	g_mysql = (st_mysql *)calloc(1, sizeof(st_mysql));
	if (strcmp(g_server->run_from, "file") == 0) {
		get_config_string(&g_server->task_file, "file", "file");
		if (NULL == g_server->task_file || -1 == access(g_server->task_file, F_OK)) {
			print_error("File '%s' is not exist.", g_server->task_file);
			exit(0);
		}

	} else if (strcmp(g_server->run_from, "mysql") == 0) {
		get_config_string(&g_mysql->host, "mysql", "host");
		get_config_string(&g_mysql->username, "mysql", "username");
		get_config_string(&g_mysql->passwd, "mysql", "passwd");
		get_config_string(&g_mysql->dbname, "mysql", "dbname");
		get_config_int(&g_mysql->port, "mysql", "g_mysql_port");
	}
}

void free_mysql(){
	free_var(g_mysql->host);
	free_var(g_mysql->username);
	free_var(g_mysql->passwd);
	free_var(g_mysql->dbname);
	free_var(g_mysql);
}

void init_mail() {
	g_mail = (st_mail *)calloc(1, sizeof(st_mail));
	get_config_string(&g_mail->server, "mail", "server");
	get_config_string(&g_mail->user, "mail", "user");
	get_config_string(&g_mail->passwd, "mail", "passwd");
	get_config_string(&g_mail->to, "mail", "to");
	get_config_int(&g_mail->port, "mail", "port");
}

void free_mail() {
	free_var(g_mail->server);
	free_var(g_mail->user);
	free_var(g_mail->passwd);
	free_var(g_mail->to);
	free_var(g_mail);
}

void free_server(){
	//删除PID
	if (0 == access(PIDFILE, F_OK)) {
		remove(PIDFILE);
	}
	//销毁全局变量
	if (NULL != task_list) {
		free_task(task_list);
		task_list = NULL;
	}
	if (NULL != g_config_file) {
		free_var(g_config_file);
		g_config_file = NULL;
	}
}

void free_resource() {
	free_main();
	free_mysql();
	free_mail();
}

void deal_task() {
	st_right_task *right_task;
	while (!g_server->shutdown) {
		pthread_mutex_lock(&LOCK_right_task);
		while (l_right_task == NULL) {
			pthread_cond_wait(&COND_right_task, &LOCK_right_task);
		}
		right_task = l_right_task;
		l_right_task = right_task->next;
		pthread_mutex_unlock(&LOCK_right_task);
		if (NULL != right_task->item) {
			deal_func func;
			func = (deal_func)right_task->item->deal_func;
			func((st_task_item *)right_task->item);
		}
		free_item(right_task->item);
		free_var(right_task);
		usleep(TASK_STEP);
	}
}

void create_threads() {
	unsigned long i = 0;
	pthread_t tid[g_server->max_threads];
	pthread_t task_tid, config_tid, mail_tid;
	// 定时加载配置线程
	pthread_create(&config_tid, NULL, (void *) load_worker, NULL);
	// 邮件队列线程
	pthread_create(&mail_tid, NULL, (void *) mail_worker, NULL);
	//任务分配线程
	pthread_create(&task_tid, NULL, (void *) task_worker, NULL);
	//创建即时任务线程
	for (i = 0; i < g_server->max_threads; i++) {
		pthread_create(&tid[i], NULL, (void *) deal_task, NULL);
	}
	pthread_join(task_tid, NULL);
	pthread_join(config_tid, NULL);
	pthread_join(mail_tid, NULL);
	for (i = 0; i < g_server->max_threads; i++) {
		pthread_join(tid[i], NULL);
	}
}
// 父进程信号处理
void sig_master(const int sig) {
	write_log("Master exit with sig %d.", sig);
	// 给进程组发送SIGTERM信号，结束子进程
	g_server->shutdown = 1;
	kill(0, SIGTERM);
	exit(0);
}

//子进程信号处理
void sig_worker(const int sig) {
	write_log("Worker exit with sig %d.", sig);
	exit(0);
}

void deal_master_sig(){
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	// 处理kill信号

	signal(SIGINT, sig_master);
	signal(SIGKILL, sig_master);
	signal(SIGQUIT, sig_master);
	signal(SIGTERM, sig_master);

	// 处理段错误信号/
	signal(SIGSEGV, sig_master);
}

void deal_worker_sig(){
	// 忽略Broken Pipe信号
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	// 处理kill信号
	signal(SIGINT, sig_worker);
	signal(SIGKILL, sig_worker);
	signal(SIGQUIT, sig_worker);
	signal(SIGTERM, sig_worker);
	// 处理段错误信号
	signal(SIGSEGV, sig_worker);
}

void create_child(void) {
	pid_t worker_pid_wait;
	pid_t worker_pid = fork();

	if (worker_pid < 0) {
		print_error("Could start worker.\n");
		exit(0);
	}
	// 父进程内容
	if (worker_pid > 0) {
		deal_master_sig();
		// 如果子进程终止，则重新派生新的子进程
		while (1) {
			worker_pid_wait = wait(NULL);
			if (worker_pid_wait < 0) {
				write_log("Worker will restart now.");
				worker_pid = fork();
				if (worker_pid == 0) {
					break;
				}
			}
			usleep(100000);
		}
	}
	//子进程处理
	deal_worker_sig();
}

void create_pid_file(void) {
	FILE *fp_pidfile;
	fp_pidfile = fopen(PIDFILE, "w");
	fprintf(fp_pidfile, "%d\n", getpid());
	fclose(fp_pidfile);
}

void daemonize(void) {
	pid_t pid = fork();
	if (pid < 0) {
		print_error("Could not daemonize.\n");
		exit(0);
	}
	if (pid > 0) {
		exit(0);
	}
	setsid();
	umask(0);
	int i;
	for (i = 0; i < NOFILE; i++) {
		close(i);
	}
}

void opt(int argc, char *argv[], char *envp[]){
	// 输入参数格式定义
	struct option long_options[] = {
			{ "daemon", 0, 0, 'd' },
			{ "config", 1, 0,'c' },
			{ "version", 0, 0, 'v' },
			{ "help", 0, 0, 'h' },
			{ NULL, 0,0, 0 } };

	if (1 == argc) {
		print_error("Please use %s --help\n", argv[0]);
		exit(1);
	}

	// 读取参数
	int c;
	while ((c = getopt_long(argc, argv, "v:t:c:dh", long_options, NULL)) != -1) {
		switch (c) {
		case 'd':
			g_server->daemon = 1;
			break;
		case 'c':
			spr_strcpy(&g_config_file, optarg);
			break;
		case 'v':
			printf("%s\n", VERSION);
			exit(0);
			break;
		case 'h':
		default:
			usage();
			exit(0);
			break;
		}
	}
	// 参数判断
	if (optind != argc) {
		print_error("Try `%s --help' for more information.\n", argv[0]);
		exit(0);
	}
}

void init_server(){
	g_server = (st_server *)calloc(1, sizeof(st_server));
	g_server->daemon = 0;
	g_server->mail_count = 0;
	g_server->shutdown = 0;
	g_server->max_threads = 1;
	g_server->notice_mail = "off";
}

// 主模块
int main(int argc, char *argv[], char *envp[]) {
	init_server();
	opt(argc, argv, envp);

	init_main();
	init_mysql();
	if (strcmp(g_server->notice_mail, "on") == 0) {
		init_mail();
	}

	if (g_server->daemon) {
		daemonize();
	}
	create_pid_file();
	create_child();
	curl_global_init(CURL_GLOBAL_ALL);
	// 初始化任务列表
	task_list = (lt_task_list *) calloc(1, sizeof(lt_task_list));
	if (NULL == task_list) {
		write_log("Could not init tasklist.");
	};
	create_threads();

	//清除curl DNS共享
	curl_share_cleanup(share_handle);
	curl_global_cleanup();
	//释放资源
	free_resource();
	free_server();
	return 0;
}
