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

#include <curl/curl.h>
#include <curl/easy.h>

#include <mysql/mysql.h>

#include "library/task.h"
#include "library/tool.h"

#define PIDFILE  "./cron.pid"
#define VERSION  "1.0"

// 函数声明
size_t Curl_Callback(void *ptr, size_t size, size_t nmemb, void *data);
void Curl_Request(int task_id, char *command, int timeout);
static void kill_signal_master(const int signal);
static void kill_signal_worker(const int signal);
void task_file_load(const char *config_file);
void task_mysql_load();
static void task_worker();
static void config_worker();
static void mail_worker();
void task_log(int id, int ret, char* msg);

/*********************************************/
char sms_url[BUFSIZ] = "http://market/sms.php";
char host[BUFSIZ] = "10.249.198.235";
char username[BUFSIZ] = "root";
char passwd[BUFSIZ] = "123456";
char dbname[BUFSIZ] = "market_test";
int port = 3306;
/*********************************************/

// 任务节点
TaskList *taskList = NULL;
// 任务配置路径
char *config_file = NULL;
// 同步配置时间
int sync_config_time = 60 * 5;
// 邮件队列间隔时间
int send_mail_time = 60 * 5;

//任务信号标识
pthread_cond_t has_task = PTHREAD_COND_INITIALIZER;
// 任务锁
pthread_mutex_t task_lock = PTHREAD_MUTEX_INITIALIZER;

//邮件信号标识
pthread_cond_t has_mail = PTHREAD_COND_INITIALIZER;
//邮件锁
pthread_mutex_t mail_lock = PTHREAD_MUTEX_INITIALIZER;

/* 请求返回数据结构 */
struct ResponseStruct {
	char *responsetext;
	size_t size;
};


/* 邮件队列结构 */
struct MAIL_QUEUE_ITEM{   
    char *ukey;
	char *subject;
	char *content;
    TAILQ_ENTRY(MAIL_QUEUE_ITEM) entries;   
};
TAILQ_HEAD(, MAIL_QUEUE_ITEM) mail_queue;

/* 帮助信息 */
static void usage(){
	printf("author raink.kid@gmail.com\n" \
		 "-h, --help     display this message then exit.\n" \
		 "-v, --version  display version information then exit.\n" \
		 "-c, --config <path>  task config file path.\n" \
		 "-d, --daemon  run as a daemon.\n\n");
}

/* Curl回调处理函数 */
size_t Curl_Callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	struct ResponseStruct *mem = (struct ResponseStruct *) data;
	// 分配内存
	mem->responsetext = realloc(mem->responsetext, mem->size + realsize + 1);
	if (mem->responsetext == NULL) {
		fprintf(stderr, "%s\n", "Responsetext malloc error.");
	}
	memcpy(&(mem->responsetext[mem->size]), ptr, realsize);
	mem->size += realsize;
	mem->responsetext[mem->size] = '\0';
	return realsize;
}

/* 邮件队列是否唯一 */
bool mail_queue_exist(char *ukey){
	bool isexist = false;
	// 初始化一个邮件
	struct MAIL_QUEUE_ITEM *tmp_item;
	tmp_item = malloc(sizeof(struct MAIL_QUEUE_ITEM));

	char tmp_ukey[BUFSIZ] = {0x00};
	sprintf(tmp_ukey, "%s", ukey);
	
	// 第一个邮件
	tmp_item = TAILQ_FIRST(&mail_queue);
	while(NULL != tmp_item){
//		fprintf(stderr, "\n%d\n", strcmp(tmp_item->ukey, tmp_ukey));
		// 如果ukey已经存在
		if(strcmp(tmp_item->ukey, tmp_ukey) == 0){
			isexist = true;
			break;
		}
		// 下一个
		tmp_item=TAILQ_NEXT(tmp_item, entries);
	}
	return isexist;
}

//记录日志
void task_log(int task_id, int ret, char* msg){
	
	// mysql连接
	MYSQL mysql_conn;
	char sql[BUFSIZ] = {0x00};
	char *mmsg;

	struct tm *p;
	time_t timep;
	time(&timep);
	p=localtime(&timep); /*取得当地时间*/
	mmsg = malloc(strlen(msg) + 1);
	strncpy(mmsg, msg, strlen(msg));

	if (mysql_library_init(0, NULL, NULL)) {
	    fprintf(stderr, "could not initialize MySQL library\n");
	 }

	// mysql 初始化连接
	if (mysql_init(&mysql_conn) == NULL) {
		fprintf(stderr, "%s\n", "Mysql Initialization fails.");
		mysql_close(&mysql_conn);
	}

	// mysql连接
	if (mysql_real_connect(&mysql_conn, host, username, passwd, dbname, port, NULL, 128) == NULL) {
		fprintf(stderr, "%s\n", "Mysql Connection fails.");
		mysql_close(&mysql_conn);
	}
	// 查询sql
	sprintf(sql, "INSERT INTO mk_timeproc_log VALUES('', %d,%d,'%s' ,'%d-%d-%d %d:%d:%d')", task_id, ret, msg, (1900+p->tm_year),( 1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
//	fprintf(stderr, "%s\n", sql);
	// 查询结果
	if (mysql_query(&mysql_conn, sql) != 0) {
		fprintf(stderr, "%s\n", "Mysql Query fails.");
		mysql_close(&mysql_conn);
		return;
	}
	free(mmsg);
	// 关闭mysql连接
	mysql_close(&mysql_conn);

	mysql_library_end();
}

/* 发送请求 */
void Curl_Request(int task_id, char *command, int timeout) {
		int ret = 0;
		CURL *curl_handle = NULL;
		CURLcode response;
		char *url;
		int ukey;
		//请求地址
		url = malloc(strlen(command));
		sprintf(url, "%s", command);
		// 信息结构体
		struct ResponseStruct chunk;
		chunk.responsetext = NULL;
		chunk.size = 0;
		// 邮件队列item
	    	struct MAIL_QUEUE_ITEM *item;
		item = malloc(sizeof(struct MAIL_QUEUE_ITEM));

		fprintf(stderr, "%s", url);
		// curl 选项设置
		curl_handle = curl_easy_init();
		if (curl_handle != NULL) {
			curl_easy_setopt(curl_handle, CURLOPT_URL, url);
			curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, timeout * 60);
			// 回调设置
			curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, Curl_Callback);
			curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &chunk);
			response = curl_easy_perform(curl_handle);
		}

		// 请求响应处理
		if ((response == CURLE_OK) && chunk.responsetext &&
			(strstr(chunk.responsetext, "__programe_run_succeed__") != 0)) {
			fprintf(stderr, "...success\n");
			ret = 1;
		} else {
			// 队列去重
			if(false == mail_queue_exist(url)){
				char *ukey;
				char *subject;
				char *content;

				ukey = malloc(strlen(url) + 1);
				subject = malloc(strlen(url) + 50);
				content = malloc(strlen(chunk.responsetext) + 50);

				sprintf(ukey, "%s", url);
				sprintf(subject, "Task Error With %s", url);
				sprintf(content, "Errors : %s", chunk.responsetext);

				// 邮件队列item
				struct MAIL_QUEUE_ITEM *item;
				item = malloc(sizeof(struct MAIL_QUEUE_ITEM));

				item->ukey = malloc(strlen(ukey) + 1);
				item->subject = malloc(strlen(subject) + 1);
				item->content = malloc(strlen(content) + 1);

				memcpy(item->ukey, ukey , strlen(ukey));
				memcpy(item->subject, subject , strlen(subject));
				memcpy(item->content, content , strlen(content));

				pthread_mutex_lock(&mail_lock);
				// 写入邮件队列
				TAILQ_INSERT_TAIL(&mail_queue, item, entries);

				pthread_mutex_unlock(&mail_lock);
				pthread_cond_signal(&has_mail);
				free(subject);
				free(content);
			}
			fprintf(stderr, "...failed\n");
		}
		//记录日志
		task_log(task_id, ret, chunk.responsetext);
		// 释放返回信息
		if (chunk.responsetext) {
			free(chunk.responsetext);
		}
		// 释放curl句柄
		curl_easy_cleanup(curl_handle);
		curl_global_cleanup();
		free(url);
}

/* 任务处理线程 */
static void task_worker() {
//	pthread_detach(pthread_self());

	// 开始处理
	int delay = 1;
	TaskItem *temp;
	while(1){
		pthread_mutex_lock(&task_lock);
		// 等待任务处理信号
		if(NULL == taskList->head) {
			pthread_cond_wait(&has_task, &task_lock);
		}
		// 现在时间
		time_t nowTime = GetNowTime();
		while (NULL != (temp = taskList->head)) {
/*			fprintf(
					stderr,
					"prev=%p, next=%p,self=%p, starTime=%ld,endTime=%ld,nextTime=%ld,times=%d,fre=%d,command=%s\n",
					temp->prev, temp->next, temp, temp->startTime,
					temp->endTime, temp->nextTime, temp->times,
					temp->frequency, temp->command);*/
			// 大于当前时间跳出
			if (nowTime < temp->nextTime) {
				delay = temp->nextTime - nowTime;
				break;
			}
			// 执行任务
			//system(command);
			Curl_Request(temp->task_id, temp->command, temp->timeout);

			(temp->runTimes)++;
			if (temp->next) {
				// 删除头部 
				taskList->head = temp->next;
				temp->next->prev = NULL;
				// 尾巴 
				if (temp == taskList->tail) {
					taskList->tail = temp->prev;
				}
				// 添加 
				if (0 == temp->times || temp->runTimes < temp->times) {
					temp->prev = NULL;
					temp->next = NULL;
					temp->nextTime = nowTime + temp->frequency;
					// 重新添加
					Task_Update(temp, taskList);
				} else {
					Item_Free(temp);
					temp = NULL;
				}
			} else {
				// 如果已经达到执行次数,抛出队列
				if (temp->times > 0 && temp->runTimes > temp->times) {
					Item_Free(temp);
					temp = NULL;
					taskList->head = NULL;
					taskList->tail = NULL;
					free(taskList);
					taskList = NULL;
					delay = 1;
				} else {
					temp->nextTime = nowTime + temp->frequency;
					delay = temp->frequency;
				}
				break;
			}
		}
		pthread_mutex_unlock(&task_lock);
		if (NULL != taskList) {
			sleep(delay);
		} else {
			break;
		}
	}
}

/* 邮件队列线程 */
static void mail_worker(){
//	pthread_detach(pthread_self());
	
	// 邮件结构
	struct MAIL_QUEUE_ITEM *tmp_item;
	tmp_item = malloc(sizeof(tmp_item));

	while(1){
		pthread_mutex_lock(&mail_lock);
		//从邮件队列头取邮件信息
		tmp_item = TAILQ_FIRST(&mail_queue);
		//等待has_mail处理信号
		if(NULL == tmp_item) {
			pthread_cond_wait(&has_mail, &mail_lock);
		}
		if(NULL != tmp_item){

			char *subject;
			char *content;

			subject = malloc(strlen(tmp_item->subject) + 1);
			content = malloc(strlen(tmp_item->content) + 1);

			strncpy(subject, tmp_item->subject, strlen(tmp_item->subject));
			strncpy(content, tmp_item->content, strlen(tmp_item->content));
			//发送短信
			send_notice_sms(subject, content);
			// 踢出除队列
			TAILQ_REMOVE(&mail_queue, tmp_item, entries);
			tmp_item=TAILQ_NEXT(tmp_item, entries);

			free(subject);
			free(content);
		}
		pthread_mutex_unlock(&mail_lock);
		sleep(send_mail_time);
	}
}

/* 同步配置线程 */
static void config_worker() {
//	pthread_detach(pthread_self());

	// 重新初始化队列
	taskList->count = 0;
	taskList->head = NULL;
	taskList->tail = NULL;

	while(1) {
		task_mysql_load();
//		task_file_load(config_file);
		sleep(sync_config_time);
	}
}

/* 父进程信号处理 */
static void kill_signal_master(const int signal) {
	//删除PID
	if (0 == access(PIDFILE, F_OK)) {
		remove(PIDFILE);
	}
	//销毁配置文件内存分配
	if (NULL != config_file) {
		free(config_file);
		config_file = NULL;
	}
	exit(EXIT_SUCCESS);
}

/*子进程信号处理*/
static void kill_signal_worker(const int signal) {

	//销毁任务
	if (NULL != taskList) {
		Task_Free(taskList);
		taskList = NULL;
	}
	//释放子进程
	pthread_exit(0);

	exit(EXIT_SUCCESS);
}

/* 文件配置计划任务加载 */
void task_file_load(const char *config_file) {
	FILE *fp;
	fp = fopen(config_file, "r");
	if(NULL == fp){
		fprintf(stderr, "%s\n", "Open config file faild.");
	}
	char line[BUFSIZ] = { 0x00 };
	while(NULL != fgets(line, BUFSIZ, fp)){
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
		sscanf(
				line,
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

		taskItem->frequency = taskItem->frequency * 60;

		taskItem->startTime = mktime(&_stime);
		taskItem->endTime = mktime(&_etime);

		// 当前时间
		time_t nowTime = GetNowTime();

		// 如果已经结束直接下一个
		if (taskItem->endTime <= nowTime || taskItem->nextTime > taskItem->endTime) {
			Item_Free(taskItem);
			continue;
		}

		// 计算下次运行点
		if (taskItem->nextTime == 0) {
			int st = ceil((nowTime - taskItem->startTime) / taskItem->frequency);
			taskItem->nextTime = taskItem->startTime + ((st + 1)
					* taskItem->frequency);
		}
		while (taskItem->nextTime <= nowTime) {
			taskItem->nextTime += taskItem->frequency;
		}
		/*fprintf(stderr, "prev=%p, next=%p,self=%p, starTime=%ld,endTime=%ld,nextTime=%ld,times=%d,frequency=%d,command=%s\n",
		 taskItem->prev, taskItem->next, taskItem, taskItem->startTime,
	 taskItem->endTime, taskItem->nextTime, taskItem->times,
		 taskItem->frequency, taskItem->command);*/
		// 更新到任务链表
		pthread_mutex_lock(&task_lock);
		Task_Update(taskItem, taskList);
		pthread_mutex_unlock(&task_lock);
		pthread_cond_signal(&has_task);
	}
	fclose(fp);
}

/* mysql任务加载 */
void task_mysql_load() {

	MYSQL mysql_conn;
	// mysql连接
	MYSQL_RES *mysql_result;
	MYSQL_ROW mysql_row;
	char sql[BUFSIZ] = {0x00};
	int row, row_num;

	char start_time[BUFSIZ] = {0x00};
	char end_time[BUFSIZ] = {0x00};
	char command[BUFSIZ] = {0x00};

	if (mysql_library_init(0, NULL, NULL)) {
	    fprintf(stderr, "could not initialize MySQL library\n");
	 }
	// mysql 初始化连接
	if (mysql_init(&mysql_conn) == NULL) {
		fprintf(stderr, "%s\n", "Mysql Initialization fails.");
		mysql_close(&mysql_conn);
	}

	// mysql连接
	if (mysql_real_connect(&mysql_conn, host, username, passwd, dbname, port, NULL, 128) == NULL) {
		fprintf(stderr, "%s\n", "Mysql Connection fails.");
		mysql_close(&mysql_conn);
	}
	// 查询sql
	sprintf(sql, "%s", "SELECT * FROM mk_timeproc");

	// 查询结果
	if (mysql_query(&mysql_conn, sql) != 0) {
		fprintf(stderr, "%s\n", "Mysql Query fails.");
		mysql_close(&mysql_conn);
		return;
	}

	// 获取结果集和条数
	mysql_result = mysql_store_result(&mysql_conn);
	row_num = mysql_num_rows(mysql_result);

	// 创建并初始化一个新节点
	TaskItem *taskItem;
	taskItem = (TaskItem *) malloc(sizeof(TaskItem));
	if(taskItem == NULL){
		fprintf(stderr, "%s\n", "taskItem malloc failed.");
		free(taskItem);
	}

	taskItem->next = NULL;
	taskItem->prev = NULL;
	taskItem->runTimes = 0;

	// 取数据
	for (row = 0; row < row_num; row++) {
		mysql_row = mysql_fetch_row(mysql_result);

		// 开始,结束时间
		struct tm _stime, _etime;
		TaskItem *taskItem;
		taskItem = (TaskItem *) malloc(sizeof(TaskItem));
		assert(NULL != taskItem);

		taskItem->next = NULL;
		taskItem->prev = NULL;
		taskItem->runTimes = 0;

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

		taskItem->frequency = atoi(mysql_row[3]) * 60;
		taskItem->times = atoi(mysql_row[8]);
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

		// 如果已经结束直接下一个
		if (taskItem->endTime <= nowTime || taskItem->nextTime > taskItem->endTime) {
			continue;
		}

		// 计算下次运行点
		if (taskItem->nextTime == 0) {
			int perst = ceil((nowTime - taskItem->startTime) / taskItem->frequency);
			taskItem->nextTime = taskItem->startTime + ((perst + 1) * taskItem->frequency);
		}
		while (taskItem->nextTime <= nowTime) {
			taskItem->nextTime += taskItem->frequency;
		}
		/*fprintf(
				stderr,
				"prev=%p, next=%p,self=%p, starTime=%ld,endTime=%ld,nextTime=%ld,times=%d,fre=%d,command=%s\n",
				taskItem->prev, taskItem->next, taskItem, taskItem->startTime,
				taskItem->endTime, taskItem->nextTime, taskItem->times,
				taskItem->frequency, taskItem->command);*/
		// 更新到任务链表
		pthread_mutex_lock(&task_lock);
		Task_Update(taskItem, taskList);
		pthread_mutex_unlock(&task_lock);
		pthread_cond_signal(&has_task);
	}
	// 释放结果集
	mysql_free_result(mysql_result);
	// 关闭mysql连接
	mysql_close(&mysql_conn);

	mysql_library_end();
}

/* 主模块 */
int main(int argc, char *argv[], char *envp[]) {
	bool daemon = false;

	// 输入参数格式定义
	struct option long_options[] = {
			{ "daemon", 0, 0, 'd' },
			{ "help", 0, 0,	'h' },
			{ "config", 1, 0, 'c' },
			{ "version", 0, 0, 'v' },
			{ 0, 0, 0, 0 } };

	if (1 == argc) {
		fprintf(stderr, "Please use %s --help\n", argv[0]);
		exit(1);
	}

	// 读取参数
	int c;
	while ((c = getopt_long(argc, argv, "dhc:v", long_options, NULL)) != -1) {
		switch (c) {
		case 'd':
			daemon = true;
			break;
		case 'c':
			config_file = malloc( strlen( optarg ) + 1 );
			assert( config_file != NULL );
			strcpy( config_file, optarg );
			break;
		case 'v':
			printf("%s\n\n", VERSION);
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
		fprintf(stderr,	"Too many arguments\nTry `%s --help' for more information.\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// 读取任务配置文件
	if (NULL == config_file || -1 == access(config_file, F_OK)) {
		fprintf(stderr,	"Please use task config: -c <path> or --config <path>\n\n");
		exit(EXIT_FAILURE);
	}

	// 如果加了-d参数，以守护进程运行
	if (daemon == true) {
		pid_t pid = fork();
		if (pid < 0) {
			fprintf(stderr, "Fork failured\n\n");
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
	// 将进程号写入PID文件
	FILE *fp_pidfile;
	fp_pidfile = fopen(PIDFILE, "w");
	fprintf(fp_pidfile, "%d\n", getpid());
	fclose(fp_pidfile);

	// 派生子进程（工作进程）
	pid_t worker_pid_wait;
	pid_t worker_pid = fork();

	// 如果派生进程失败，则退出程序
	if (worker_pid < 0) {
		fprintf(stderr, "Error: %s:%d\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}

	// 父进程内容
	if (worker_pid > 0) {
		// 处理父进程接收到的kill信号

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

	// 初始化任务列表
	taskList = (TaskList *) malloc(sizeof(TaskList));
	if(NULL == taskList){
		fprintf(stderr, "%s\n", "tasklist malloc failed.");
	};
	taskList->count = 0;
	taskList->head = NULL;
	taskList->tail = NULL;

	// 初始化邮件队列
	TAILQ_INIT(&mail_queue);

	pthread_t config_tid, task_tid, mail_tid;
	// 定时加载配置张线程
	pthread_create(&config_tid, NULL, (void *) config_worker, NULL);
	// 创建计划任务线程
	pthread_create(&task_tid, NULL, (void *) task_worker, NULL);
	// 创建邮件队列线程
	pthread_create(&mail_tid, NULL, (void *) mail_worker, NULL);

	pthread_join ( config_tid, NULL );
	pthread_join ( task_tid, NULL );
	pthread_join ( mail_tid, NULL );

	pthread_exit(0);

	return 0;
}

