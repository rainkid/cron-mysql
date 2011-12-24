/*
 * main.c
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
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <assert.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <math.h>
#include <sys/prctl.h>
#include <netinet/in.h> 
#include <sys/socket.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <mysql/mysql.h>

#include "list.h"
#include "tool.h"
#include "config.h"
#include "base64.h"
#include "mail.h"
#include "define.h"

/*******************************************************************/
// 函数声明
void write_log(const char *fmt,  ...);
int send_notice_mail(char *subject, char *content);
size_t curl_callback(void *ptr, size_t size, size_t nmemb, void *data);
void task_log(int task_id, int ret, char* msg);
void curl_request(TaskItem *task_item);
void *pull_one_item(void *item);
void task_worker();
void load_worker();
void task_file_load(const char *task_file);
void task_mysql_load();
/*******************************************************************/
//全局变量
char *g_config_file = NULL;
// 任务节点
TaskList *task_list = NULL;
//mysql参数
MysqlParams *g_mysql_params = NULL;
//邮件参数
MailParams *g_mail_params = NULL;
//全局变量
GlobalParams *global = NULL;
//任务锁
pthread_mutex_t task_lock = PTHREAD_MUTEX_INITIALIZER;

TaskItem *task_right_list[THREAD_MAX];
/*******************************************************************/
/* 帮助信息 */
void usage() {
	printf("author raink.kid@gmail.com\n" \
		 "-h, --help     display this message then exit.\n" \
		 "-v, --version  display version information then exit.\n" \
		 "-c, --config   <path>  task config file path.\n" \
		 "-d, --daemon   run as a daemon.\n\n");
}
/*******************************************************************/
//日志函数
void write_log(const char *fmt,  ...) {
	char msg[BUFSIZE];
	FILE * fp = NULL;
	struct stat buf;
	stat(LOG_FILE, &buf);
	if (buf.st_size > MAX_LOG_SIZE) {
		rename(LOG_FILE, BACK_LOG_FILE);
	}

	//时间
	struct tm *p;
	time_t timep;
	time(&timep);
	p=localtime(&timep);
	//参数处理
	va_list  va;
	va_start(va, fmt);
	vsprintf(msg, fmt, va);
	va_end(va);
	//日志写入
	fp = fopen(LOG_FILE , "ab+" );
	if (fp) {
		fprintf(fp,"%04d-%02d-%02d %02d:%02d:%02d %s\n",(1900+p->tm_year),( 1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, msg);
	}
	fclose(fp);
}
/*******************************************************************/
/* curl回调处理函数 */
size_t curl_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	struct Response *mem = (struct Response *) data;
	mem->responsetext = malloc(mem->size + realsize + 1);
	if (NULL == mem->responsetext) {
		write_log("responsetext malloc error.");
	}
	memcpy(&(mem->responsetext[mem->size]), ptr, realsize);
	mem->size += realsize;
	mem->responsetext[mem->size] = '\0';
	return realsize;
}
/*******************************************************************/
/*发送通知邮件*/
int send_notice_mail(char *subject, char *content) {
    int ret = 0;

    char *m_subject;
    char *m_content;
    m_subject = malloc(strlen(subject));
    m_content = malloc(strlen(content));

    strncpy(m_subject, subject, strlen(subject));
    strncpy(m_content, content, strlen(content));

    struct st_char_arry to_addrs[1];
	//收件人列表
    	to_addrs[0].str_p = g_mail_params->to;
    struct st_char_arry att_files[0];
	//附件列表
  	att_files[0].str_p="";
	struct st_mail_msg_ mail;
	init_mail_msg(&mail);
	mail.authorization = AUTH_SEND_MAIL;
	mail.server = g_mail_params->server;
	mail.port = g_mail_params->port;
	mail.auth_user = g_mail_params->user;
	mail.auth_passwd= g_mail_params->passwd;
	mail.from = g_mail_params->user;
	mail.from_subject = "TaskError@163.com";
	mail.to_address_ary = to_addrs;
	mail.to_addr_len = 1;
	mail.mail_style_html = HTML_STYLE_MAIL;
	mail.priority = 3;
	mail.att_file_len = 2;
	mail.att_file_ary = att_files;
	mail.subject = m_subject;
	mail.content = m_content;
	ret = send_mail(&mail);
	if (ret != 0) {
		write_log("send mail with error : %d.", ret);
	}
	free(m_subject);
	free(m_content);
	return ret;
}
/*******************************************************************/
void mysql_begin(MYSQL *mysql_conn){
	mysql_thread_init();
	// mysql 初始化连接
	if (NULL == mysql_init(mysql_conn)) {
		write_log("mysql initialization fails.");
	}
	// mysql连接
	if(NULL == mysql_real_connect(mysql_conn, g_mysql_params->host, g_mysql_params->username, g_mysql_params->passwd, g_mysql_params->dbname, g_mysql_params->port, NULL, 128)){
		write_log("mysql connection fails.");
	}
}
/*******************************************************************/
void mysql_end(MYSQL *mysql_conn){
	mysql_close(mysql_conn);
	mysql_thread_end();
}
/*******************************************************************/
//记录日志
void task_log(int task_id, int ret, char* msg) {
	MYSQL mysql_conn;
	char sql[BUFSIZE] = {0x00};
	char upsql[BUFSIZE] = {0x00};
	int mtask_id = task_id;
	char *mmsg;

	struct tm *p;
	time_t timep;
	time(&timep);
	p = localtime(&timep);
	mmsg = malloc(strlen(msg)+1);
	sprintf(mmsg, "%s", msg);
	mysql_begin(&mysql_conn);
	//更新执行时间
	sprintf(upsql, "UPDATE mk_timeproc SET last_run_time='%04d-%02d-%02d %02d:%02d:%02d' WHERE id=%d", (1900+p->tm_year),( 1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, task_id);
	if (mysql_query(&mysql_conn, upsql) != 0) {
		write_log("update last_run_time fails.");
	}
	//添加日志
	sprintf(sql, "INSERT INTO mk_timeproc_log VALUES('', %d,%d,\"%s\" ,'%04d-%02d-%02d %02d:%02d:%02d')", task_id, ret, msg, (1900+p->tm_year),( 1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
	if (mysql_query(&mysql_conn, sql) != 0) {
		write_log("insert logs fails.");
	}
	free(mmsg);
	mysql_end(&mysql_conn);
}

/*******************************************************************/

//邮件队列
void mail_worker(){
	TaskItem *task_item;
	for (;;) {
		pthread_mutex_lock(&task_lock);
		if (NULL != task_list && task_list->count > 0) {
			task_item = task_list->head;
			while (NULL !=task_item) {
				if (true == task_item->mail) {
					char subject[BUFSIZE] = {0x00};
					char content[BUFSIZE] = {0x00};
					write_log("send an e-mail right now.");
					sprintf(subject, "CTask with some error!");
					sprintf(content, "The task_id is [%d]\r\nThe task url : [%s]\r\nPlease check it right now", task_item->task_id, task_item->command);
					send_notice_mail(subject, content);
					task_item->mail = false;
				}
				task_item = task_item->next;
			}
		}
		pthread_mutex_unlock(&task_lock);
		usleep(SEND_MAIL_TIME);
	}
}
/*******************************************************************/
//
void *pull_one_item(void *item) {
	int ret = 0;
	char *url;
	CURL *curl_handle = NULL;

	struct Response chunk;
	chunk.responsetext = NULL;
	chunk.size = 0;

	CURLcode response;
	curl_handle = curl_easy_init();

	TaskItem *task_item;
	task_item = (TaskItem *)item;
	url = malloc(strlen(task_item->command) + 1);
	sprintf(url, "%s", task_item->command);
	if (curl_handle != NULL) {

		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, task_item->timeout * TIME_UNIT);
		// 回调设置
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curl_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &chunk);

		response = curl_easy_perform(curl_handle);
	}else{
		write_log("curl handler is null.");
	}
	// 请求响应处理
	if ((response == CURLE_OK) && chunk.responsetext &&
		(strstr(chunk.responsetext, "__programe_run_succeed__") != 0)) {
		write_log("%s...success", url);
		ret = 1;
	} else {
		task_item->mail = true;
		write_log("%s...fails", url);
	}
	//记录日志
	if (strcmp(global->run_type, "mysql") == 0) {
		if(NULL != chunk.responsetext){
			task_log(task_item->task_id, ret, chunk.responsetext);
		}
	}

	if (NULL != chunk.responsetext) {
		free(chunk.responsetext);
	}
	task_item->status = 0;
	free(url);
	curl_easy_cleanup(curl_handle);
}
/*******************************************************************/
/* 任务处理线程 */
void task_worker() {
	TaskItem *temp;
	for (;;) {
		pthread_mutex_lock(&task_lock);
		if (NULL != task_list && task_list->count > 0) {
			while (NULL != (temp = task_list->head)) {
				// 大于当前时间跳出
				time_t nowTime = GetNowTime();
				if (nowTime < temp->nextTime || temp->status == 1) {
					break;
				}
				int i = 0 ;
				for(i=0; i<THREAD_MAX; i++) {
					if (task_right_list[i] == NULL) {
						task_right_list[i] = temp;
						break;
					}
				}

				(temp->runTimes)++;
				if (temp->next) {
					task_list->head = temp->next;
					temp->next->prev = NULL;
					(task_list->count)--;
					if (temp == task_list->tail) {
						task_list->tail = temp->prev;
					}
					if (0 == temp->times || temp->runTimes < temp->times) {
						temp->prev = NULL;
						temp->next = NULL;
						temp->nextTime += temp->frequency;
						task_update(temp, task_list);
					} else {
						item_free(temp, task_list);
						temp = NULL;
					}
				} else {
					// 已经达到执行次数,抛出队列
					if (temp->times > 0 && temp->runTimes > temp->times) {
						item_free(temp, task_list);
						temp = NULL;

						(task_list->count)--;
						task_list->head = NULL;
						task_list->tail = NULL;
						task_free(task_list);
						task_list = NULL;
					} else {
						temp->nextTime += temp->frequency;
					}
				}
			}
			write_log("%d tasks in task list.", task_list->count);
		}else{
			write_log("task list is null.");
		}
		pthread_mutex_unlock(&task_lock);
		usleep(TASK_STEP);
	}
}
/*******************************************************************/
/* 同步配置线程 */
void load_worker() {
	for (;;) {
		// 加载任务到新建列表中
		if (strcmp(global->run_type, "file") == 0) {
			task_file_load(global->task_file);
		} else if (strcmp(global->run_type, "mysql") == 0) {
			task_mysql_load();
		}
		usleep(SYNC_CONFIG_TIME);
	}
}
/*******************************************************************/
// 父进程信号处理
void kill_signal_master(const int signal) {
	//删除PID
	if (0 == access(PIDFILE, F_OK)) {
		remove(PIDFILE);
	}
	//销毁配置文件内存分配
	if (NULL != g_config_file) {
		free(g_config_file);
		g_config_file = NULL;
	}
	if (NULL != g_mysql_params) {
		free(g_mysql_params);
		g_mysql_params = NULL;
	}
	if (NULL != g_mail_params) {
		free(g_mail_params);
		g_mail_params = NULL;
	}
	// 给进程组发送SIGTERM信号，结束子进程
	kill(0, SIGTERM);
	exit(EXIT_SUCCESS);
}

/*******************************************************************/
//子进程信号处理
void kill_signal_worker(const int signal) {
	//销毁任务
	if (NULL != task_list) {
		task_free(task_list);
		task_list = NULL;
	}
	curl_global_cleanup();
	pthread_exit(0);
	kill(0, SIGTERM);
	exit(EXIT_SUCCESS);
}

/*******************************************************************/
// 文件配置计划任务加载
void task_file_load(const char *task_file) {
	FILE *fp;
	fp = fopen(global->task_file, "r");
	if (NULL == fp) {
		write_log("open config file faild.");
	}
	char line[BUFSIZE] = { 0x00 };

	pthread_mutex_lock(&task_lock);
	//初始化任务列表
	task_init(task_list);

	while (NULL != fgets(line, BUFSIZE, fp)) {
		// 忽略空行和＃号开头的行
		if ('\n' == line[0] || '#' == line[0]) {
			continue;
		}
		line[strlen(line) + 1] = '\0';
		// 开始,结束时间
		struct tm _stime, _etime;

		// 创建并初始化一个新节点
		TaskItem *taskItem;
		taskItem = (TaskItem *) malloc(sizeof(TaskItem));

		taskItem->next = NULL;
		taskItem->prev = NULL;
		taskItem->runTimes = 0;

		// 按格式读取
		sscanf(line,
				"%04d-%02d-%02d %02d:%02d:%02d,%04d-%02d-%02d %02d:%02d:%02d,%d,%d,%[^\n]",
				&_stime.tm_year, &_stime.tm_mon, &_stime.tm_mday,
				&_stime.tm_hour, &_stime.tm_min, &_stime.tm_sec,
				&_etime.tm_year, &_etime.tm_mon, &_etime.tm_mday,
				&_etime.tm_hour, &_etime.tm_min, &_etime.tm_sec,
				&taskItem->frequency, &taskItem->times, taskItem->command);

		// 转化为时间戳
		_stime.tm_year -= 1900;
		_etime.tm_year -= 1900;
		_stime.tm_mon -= 1;
		_etime.tm_mon -= 1;

		taskItem->frequency = taskItem->frequency * TIME_UNIT;

		taskItem->startTime = mktime(&_stime);
		taskItem->endTime = mktime(&_etime);

		// 当前时间
		time_t nowTime = GetNowTime();

		// 如果已经结束直接下一个
		if (taskItem->endTime <= nowTime || taskItem->nextTime > taskItem->endTime) {
			item_free(taskItem, task_list);
			continue;
		}

		// 计算下次运行点
		int step = ceil((nowTime - taskItem->startTime) / taskItem->frequency);
		taskItem->nextTime = taskItem->startTime + ((step + 1) * taskItem->frequency);
		while (taskItem->nextTime <= nowTime) {
			taskItem->nextTime += taskItem->frequency;
		}
		// 更新到任务链表
		task_update(taskItem, task_list);
	}
	write_log("load tasks form file.");
	pthread_mutex_unlock(&task_lock);
	fclose(fp);
}
/*******************************************************************/
//mysql任务加载
void task_mysql_load() {
	MYSQL mysql_conn;
	MYSQL_RES *mysql_result;
	MYSQL_ROW mysql_row;
	char sql[BUFSIZE] = {0x00};
	int row, row_num;

	char start_time[BUFSIZE] = {0x00};
	char end_time[BUFSIZE] = {0x00};
	char command[BUFSIZE] = {0x00};

	mysql_begin(&mysql_conn);
	// 查询sql
	sprintf(sql, "%s", "SELECT * FROM mk_timeproc");

	// 查询结果
	if (mysql_query(&mysql_conn, sql) != 0) {
		write_log("mysql query fails.");
	}

	// 获取结果集和条数
	mysql_result = mysql_store_result(&mysql_conn);
	row_num = mysql_num_rows(mysql_result);

	pthread_mutex_lock(&task_lock);
	//初始化任务列表
	task_init(task_list);

	// 取数据
	for (row = 0; row < row_num; row++) {
		mysql_row = mysql_fetch_row(mysql_result);

		// 开始,结束时间
		struct tm _stime, _etime;
		TaskItem *taskItem;
		taskItem = (TaskItem *) malloc(sizeof(TaskItem));
		assert(NULL != taskItem);

		taskItem->mail = false;
		taskItem->next = NULL;
		taskItem->prev = NULL;
		taskItem->runTimes = 0;
		taskItem->status = 0;

		// 格式化开始时间
		sprintf(start_time, "%s", mysql_row[1]);
		sscanf(start_time, "%04d-%02d-%02d %02d:%02d:%02d", &_stime.tm_year,
				&_stime.tm_mon, &_stime.tm_mday, &_stime.tm_hour,
				&_stime.tm_min, &_stime.tm_sec);

		// 格式化结束时间
		sprintf(end_time, "%s", mysql_row[2]);
		sscanf(end_time, "%04d-%02d-%02d %02d:%02d:%02d", &_etime.tm_year,
				&_etime.tm_mon, &_etime.tm_mday, &_etime.tm_hour,
				&_etime.tm_min, &_etime.tm_sec);

		// 格式化命令
		sprintf(command, "%s", mysql_row[5]);
		sprintf(taskItem->command, "%s", command);

		taskItem->times = 0;
		taskItem->frequency = atoi(mysql_row[3]) * TIME_UNIT;
		taskItem->task_id = atoi(mysql_row[0]);
		taskItem->timeout = atoi(mysql_row[7]);

		// 转化为时间戳
		_stime.tm_year -= 1900;
		_etime.tm_year -= 1900;
		_stime.tm_mon -= 1;
		_etime.tm_mon -= 1;

		taskItem->startTime = mktime(&_stime);
		taskItem->endTime = mktime(&_etime);

		// 当前时间
		time_t nowTime = GetNowTime();

		// 计算下次运行点
		int step = ceil((nowTime - taskItem->startTime) / taskItem->frequency);
		taskItem->nextTime = taskItem->startTime + ((step + 1) * taskItem->frequency);

		while (taskItem->nextTime <= nowTime) {
			taskItem->nextTime += taskItem->frequency;
		}

		// 如果已经结束直接下一个
		if (taskItem->endTime <= nowTime || taskItem->nextTime > taskItem->endTime) {
			continue;
		}
		// 更新到任务链表
		task_update(taskItem, task_list);
	}
	write_log("load %d tasks from mysql.", task_list->count);

	pthread_mutex_unlock(&task_lock);
	// 释放结果集
	mysql_free_result(mysql_result);
	mysql_end(&mysql_conn);
}
/*******************************************************************/
void create_child(void){
	// 派生子进程（工作进程）
	pid_t worker_pid_wait;
	pid_t worker_pid = fork();

	// 如果派生进程失败，则退出程序
	if (worker_pid < 0) {
		fprintf(stderr, "error: %s:%d\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}

	// 父进程内容
	if (worker_pid > 0) {
		// 忽略Broken Pipe信号
		signal(SIGPIPE, SIG_IGN);

		// 处理kill信号
		signal(SIGINT, kill_signal_master);
		signal(SIGKILL, kill_signal_master);
		signal(SIGQUIT, kill_signal_master);
		signal(SIGTERM, kill_signal_master);
		signal(SIGHUP, kill_signal_master);

		// 处理段错误信号/
		signal(SIGSEGV, kill_signal_master);

		// 如果子进程终止，则重新派生新的子进程
		while (1) {
			worker_pid_wait = wait(NULL);
			if (worker_pid_wait < 0) {
				continue;
			}
			usleep(100000);
			worker_pid = fork();
			if (worker_pid == 0) {
				break;
			}
		}
	}

	/*** 子进程处理 ***/

	// 忽略Broken Pipe信号
	signal(SIGPIPE, SIG_IGN);

	// 处理kill信号
	signal(SIGINT, kill_signal_worker);
	signal(SIGKILL, kill_signal_worker);
	signal(SIGQUIT, kill_signal_worker);
	signal(SIGTERM, kill_signal_worker);
	signal(SIGHUP, kill_signal_worker);
	// 处理段错误信号
	signal(SIGSEGV, kill_signal_worker);
}
/*******************************************************************/
void create_pid_file(void) {
	FILE *fp_pidfile;
	fp_pidfile = fopen(PIDFILE, "w");
	fprintf(fp_pidfile, "%d\n", getpid());
	fclose(fp_pidfile);
}
/*******************************************************************/
void daemonize(void) {
		pid_t pid = fork();
		if (pid < 0) {
			fprintf(stderr, "fork failured\n");
			exit(EXIT_FAILURE);
		}
		if (pid > 0) {
			exit(EXIT_SUCCESS);
		}
		setsid();
		umask(0);
		int i;
		for (i = 0; i < NOFILE; i++) {
			close(i);
		}
}
void init_global_params(){
	global = (GlobalParams *)malloc(sizeof(GlobalParams));
	// 配置选项检查-运行方式
	sprintf(global->run_type, "%s", c_get_string("main", "run", g_config_file));
	if (strcmp(global->run_type, "file") != 0 && strcmp(global->run_type, "mysql") !=0 ) {
		fprintf(stderr, "error run type : %s, it's is mysql or file\n", global->run_type);
		exit(EXIT_FAILURE);
	}
	//是否开启通知
	sprintf(global->notice, "%s", c_get_string("main", "notice", g_config_file));
	if (strcmp(global->notice, "on") != 0 && strcmp(global->notice, "off") !=0 ) {
		fprintf(stderr, "error notice value : %s, it's is on or off\n", global->notice);
		exit(EXIT_FAILURE);
	}
}
/*******************************************************************/
void init_mysql_params(){
	g_mysql_params = (MysqlParams *)malloc(sizeof(MysqlParams));
	// 读取任务配置文件
	if (strcmp(global->run_type, "file") == 0) {
		sprintf(global->task_file, "%s", c_get_string("file", "file", g_config_file));
		if (NULL == global->task_file || -1 == access(global->task_file, F_OK)) {
			fprintf(stderr,	"task file is not exist.\n");
			exit(EXIT_FAILURE);
		}
	} else if (strcmp(global->run_type, "mysql") == 0) {
		sprintf(g_mysql_params->host, "%s", c_get_string("mysql", "host", g_config_file));
		sprintf(g_mysql_params->username, "%s", c_get_string("mysql", "username", g_config_file));
		sprintf(g_mysql_params->passwd, "%s", c_get_string("mysql", "passwd", g_config_file));
		sprintf(g_mysql_params->dbname, "%s", c_get_string("mysql", "dbname", g_config_file));
		g_mysql_params->port = c_get_int("mysql", "g_mysql_port", g_config_file);
	}
}
/*******************************************************************/
void init_mail_params(){
	g_mail_params = (MailParams *)malloc(sizeof(MailParams));
	sprintf(g_mail_params->server, "%s", c_get_string("mail", "server", g_config_file));
	sprintf(g_mail_params->user, "%s", c_get_string("mail", "user", g_config_file));
	sprintf(g_mail_params->passwd, "%s", c_get_string("mail", "passwd", g_config_file));
	sprintf(g_mail_params->to, "%s", c_get_string("mail", "to", g_config_file));
	g_mail_params->port = c_get_int("mail", "port", g_config_file);
}
/*******************************************************************/
void task_right(void *thread_id){
	long tid = (long)thread_id;
	for(;;) {
		TaskItem * task_item = (TaskItem *)task_right_list[tid];
	    if (task_item != NULL) {
			pull_one_item(task_item);
			task_right_list[tid] = NULL;			
		}
		usleep(100000);
	}
}
/*******************************************************************/
void task_main(){
	long i = 0;
	pthread_t tid[THREAD_MAX];
	pthread_t task_tid, config_tid, mail_tid;
	// 定时加载配置线程
	pthread_create(&config_tid, NULL, (void *) load_worker, NULL);
	// 邮件队列线程
	pthread_create(&mail_tid, NULL, (void *) mail_worker, NULL);
	//任务分配线程
	pthread_create(&task_tid, NULL, (void *) task_worker, NULL);
	//创建即时任务线程
	for(i=0; i<THREAD_MAX; i++) {
		pthread_create(&tid[i], NULL,(void *) task_right, (void *)i);		
	}
    pthread_join(task_tid, NULL);
    pthread_join(config_tid, NULL);
    pthread_join(mail_tid, NULL);
	for(i=0; i<THREAD_MAX; i++) {
		pthread_join(tid[i], NULL);
	}
}
/*******************************************************************/
void init_right_task_list(){
	int i = 0;
	for(i=0; i<THREAD_MAX; i++) {
		task_right_list[i] != NULL;
	}
}
/*******************************************************************/
// 主模块
int main(int argc, char *argv[], char *envp[]) {
	bool daemon = false;
	// 输入参数格式定义
	struct option long_options[] = {
			{ "daemon", 0, 0, 'd' },
			{ "config", 1, 0, 'c' },
			{ "version", 0, 0, 'v' },
			{ "help", 0, 0, 'h' },
			{ NULL, 0, 0, 0 }};

	if (1 == argc) {
		fprintf(stderr, "please use %s --help\n", argv[0]);
		exit(1);
	}

	// 读取参数
	int c;
	while ((c = getopt_long(argc, argv, "v:t:c:dh", long_options, NULL)) != -1) {
		switch (c) {
		case 'd':
			daemon = true;
			break;
		case 'c':
			g_config_file = malloc(strlen(optarg)+1);
			assert( NULL != g_config_file);
			sprintf( g_config_file, "%s", optarg );
			break;
		case 'v':
			printf("%s\n", VERSION);
			exit(EXIT_SUCCESS);
			break;
		case 'h':
		default:
			usage();
			exit(EXIT_SUCCESS);
			break;
		}
	}
	// 参数判断
	if (optind != argc) {
		fprintf(stderr,	"too many arguments\nTry `%s --help' for more information.\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	init_global_params();
	init_mysql_params();
	if (strcmp(global->notice, "on") == 0) {
		init_mail_params();
	}
	if (daemon == true) {
		daemonize();
	}
	create_pid_file();
	create_child();
	// 初始化任务列表
	task_list = (TaskList *) malloc(sizeof(TaskList));
	if (NULL == task_list) {
		write_log("tasklist malloc failed.");
	};
	init_right_task_list();
	task_main();
	return 0;
}
