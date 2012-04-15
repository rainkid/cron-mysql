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

#include "util.h"
#include "define.h"
#include "list.h"
#include "config.h"
#include "base64.h"
#include "mail.h"
#include "task.h"

/* 帮助信息 */
void usage() {
	printf("author raink.kid@gmail.com\n"
		"-h, --help     display this message then exit.\n"
		"-v, --version  display version information then exit.\n"
		"-c, --config   <path>  task config file path.\n"
		"-d, --daemon   run as a daemon.\n\n");
}

/* mysql连接 */
bool task_mysql_connect(MYSQL *mysql_conn) {
	/* 初始化mysql */
	my_init();
	/* 初始化mysql函数库 */
	if (mysql_library_init(0, NULL, NULL)) {
		write_log("could not initialize mysql library.");
		return false;
	}
	/* mysql 初始化连接 */
	if (NULL == mysql_init(mysql_conn)) {
		write_log("mysql_init() failed.");
		return false;
	}
	/* mysql连接 */
	if (NULL == mysql_real_connect(mysql_conn, g_mysql_params.host,
			g_mysql_params.username, g_mysql_params.passwd,
			g_mysql_params.dbname, g_mysql_params.port, NULL, 0)) {
		write_log("mysql_real_connect() failed.");
		return false;
	}
	return true;
}

/* 关闭mysql */
void task_mysql_close(MYSQL *mysql_conn){
	mysql_close(mysql_conn);
	/* 关闭mysql函数库 */
	mysql_library_end();
}

/* 邮件队列 */
void mail_worker() {
//	pthread_detach(pthread_self());
	struct s_right_mail *right_mail;
	char subject[BUFSIZE] = { 0x00 };
	char content[BUFSIZE] = { 0x00 };

	for (;;) {
		if (server.shutdown) {
			break;
		}
		if (NULL != l_right_mail) {
			pthread_mutex_lock(&LOCK_right_mail);
			right_mail = l_right_mail;
			l_right_mail = right_mail->next;
			server.mail_count--;
			pthread_mutex_unlock(&LOCK_right_mail);
			sprintf(subject, "taskserver with some error!");
			sprintf(content, "Error with [%s] \n", right_mail->content);
			send_notice_mail(subject, content);
			write_log("send an e-mail right now.");
		}
		sleep(SEND_MAIL_TIME);
	}
}

/* 发送通知邮件 */
int send_notice_mail(char *subject, char *content) {
	int ret = 0;
	struct st_char_arry to_addrs[1];
	/* 收件人列表 */
	to_addrs[0].str_p = g_mail_params.to;
	struct st_char_arry att_files[0];
	/* 附件列表 */
	att_files[0].str_p = "";
	struct st_mail_msg_ mail;
	init_mail_msg(&mail);
	mail.subject = malloc(strlen(subject) + 1);
	mail.content = malloc(strlen(content) + 1);
	mail.authorization = AUTH_SEND_MAIL;
	mail.server = g_mail_params.server;
	mail.port = g_mail_params.port;
	mail.auth_user = g_mail_params.user;
	mail.auth_passwd = g_mail_params.passwd;
	mail.from = g_mail_params.user;
	mail.from_subject = "TaskError@163.com";
	mail.to_address_ary = to_addrs;
	mail.to_addr_len = 1;
	mail.mail_style_html = HTML_STYLE_MAIL;
	mail.priority = 3;
	mail.att_file_len = 2;
	mail.att_file_ary = att_files;
	sprintf(mail.subject,"%s", subject);
	sprintf(mail.content,"%s", content);
	ret = send_mail(&mail);
	if (ret != 0) {
		write_log("send mail with error : %d.", ret);
	}
	free(mail.subject);
	free(mail.content);
	return ret;
}

/* 任务处理线程 */
void task_worker() {
//	pthread_detach(pthread_self());
	s_task_item *temp;
	struct s_right_task *right_item;
	for (;;) {
		if (server.shutdown) {
			break;
		}
		pthread_mutex_lock(&LOCK_task);
		if (NULL != task_list && task_list->count > 0) {
			temp = malloc(sizeof(s_task_item *));
			while (NULL != (temp = task_list->head)) {
				/* 大于当前时间跳出 */
				time_t nowTime = GetNowTime();
				if (nowTime < temp->nextTime) {
					break;
				}
				right_item = malloc(sizeof(struct s_right_task));
				right_item->item = temp;
				pthread_mutex_lock(&LOCK_right_task);
				right_item->next = l_right_task;
				l_right_task = right_item;
				pthread_mutex_unlock(&LOCK_right_task);
				pthread_cond_broadcast(&COND_right_task);

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
						temp->nextTime = nowTime + temp->frequency;
						task_update(temp, task_list);
					} else {
						item_free(temp, task_list);
						temp = NULL;
					}
				} else {
					/* 已经达到执行次数,抛出队列 */
					if (temp->times > 0 && temp->runTimes > temp->times) {
						item_free(temp, task_list);
						temp = NULL;

						(task_list->count)--;
						task_list->head = NULL;
						task_list->tail = NULL;
						task_free(task_list);
						task_list = NULL;
					} else {
						temp->nextTime = nowTime + temp->frequency;
					}
				}
			}
		}
		pthread_mutex_unlock(&LOCK_task);
		usleep(TASK_STEP);
	}
}
/* CURL请求处理 */
void curl_request(s_task_item *item) {
	int ret = 0;
	CURL *curl_handle;

	struct s_response chunk;
	chunk.responsetext = NULL;
	chunk.size = 0;

	CURLcode response;
	curl_handle = curl_easy_init();

	s_task_item *task_item;
	task_item = malloc(sizeof(s_task_item));
	pthread_mutex_lock(&LOCK_task);
	sprintf(task_item->command, "%s", item->command);
	task_item->task_id = item->task_id;
	pthread_mutex_unlock(&LOCK_task);

	struct s_right_mail *right_mail;

	if (curl_handle != NULL) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, task_item->command);
		curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT,
				task_item->timeout * TIME_UNIT);
		/* 回调设置 */
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, curl_callback);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &chunk);

		response = curl_easy_perform(curl_handle);
	} else {
		write_log("curl handler is null.");
	}
	/* 请求响应处理 */
	if ((response == CURLE_OK) && chunk.responsetext && (strstr(
			chunk.responsetext, "__programe_run_succeed__") != 0)) {
		write_log("%s...success", task_item->command);
		ret = 1;
	} else {
		write_log("%s...failed", task_item->command);
		if (server.mail_count < 6) {
			right_mail = malloc(sizeof(struct s_right_mail));
			sprintf(right_mail->content, "%s", task_item->command);
			pthread_mutex_lock(&LOCK_right_mail);
			right_mail->next = l_right_mail;
			l_right_mail = right_mail;
			server.mail_count++;
			pthread_mutex_unlock(&LOCK_right_mail);
		}
	}
	/* 记录日志 */
	if (strcmp(server.run_type, "mysql") == 0) {
		if (NULL != chunk.responsetext) {
			task_log(task_item->task_id, ret, chunk.responsetext);
		}
	}

	if (NULL != chunk.responsetext) {
		free(chunk.responsetext);
	}
	free(task_item);
	curl_easy_cleanup(curl_handle);
}

/* 命令管道 */
void shell_command(s_task_item *item) {
	FILE * fp;
	int ret = 0;
	s_task_item *task_item;
	char responsetext[1024];

	task_item = malloc(sizeof(s_task_item));
	pthread_mutex_lock(&LOCK_task);
	sprintf(task_item->command, "%s", item->command);
	task_item->task_id = item->task_id;
	pthread_mutex_unlock(&LOCK_task);

	struct s_right_mail *right_mail;

	fp = popen(task_item->command, "r");
	if(NULL != fgets(responsetext, sizeof(responsetext), fp)){
		if (responsetext && (strstr(responsetext, "__programe_run_succeed__") != 0)) {
			ret = 1;
			write_log("%s...success", task_item->command);
		} else {
			write_log("%s...failed", task_item->command);
			if (server.mail_count < 6) {
				right_mail = malloc(sizeof(struct s_right_mail));
				sprintf(right_mail->content, "%s", task_item->command);
				pthread_mutex_lock(&LOCK_right_mail);
				right_mail->next = l_right_mail;
				l_right_mail = right_mail;
				server.mail_count++;
				pthread_mutex_unlock(&LOCK_right_mail);
			}
		}
	}
	/* 记录日志 */
	if (strcmp(server.run_type, "mysql") == 0) {
		if (NULL != responsetext) {
			task_log(task_item->task_id, ret, responsetext);
		}
	}
	pclose(fp);
	free(task_item);
}

/* curl回调处理函数 */
size_t curl_callback(void *ptr, size_t size, size_t nmemb, void *data) {
	size_t realsize = size * nmemb;
	struct s_response *mem = (struct s_response *) data;
	mem->responsetext = malloc(mem->size + realsize + 1);
	if (NULL == mem->responsetext) {
		write_log("responsetext malloc error.");
	}
	memcpy(&(mem->responsetext[mem->size]), ptr, realsize);
	mem->size += realsize;
	mem->responsetext[mem->size] = '\0';
	return realsize;
}

/* 计划日志 */
void task_log(int task_id, int ret, char* msg) {
	MYSQL mysql_conn;
	char sql[2048] = { 0x00 };
	char upsql[1024] = { 0x00 };

	struct tm *p;
	time_t timep;
	time(&timep);
	p = localtime(&timep);

	if (task_mysql_connect(&mysql_conn)) {
		/* 更新执行时间 */
		sprintf(
				upsql,
				"UPDATE mk_timeproc SET last_run_time='%04d-%02d-%02d %02d:%02d:%02d' WHERE id=%d",
				(1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday,
				p->tm_hour, p->tm_min, p->tm_sec, task_id);
		if (mysql_query(&mysql_conn, upsql) != 0) {
			write_log("update task last_run failed.");
		}
		/* 添加日志 */
		sprintf(
				sql,
				"INSERT INTO mk_timeproc_log VALUES('', %d,%d,'%s' ,'%04d-%02d-%02d %02d:%02d:%02d')",
				task_id, ret, msg, (1900 + p->tm_year), (1 + p->tm_mon),
				p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
		if (mysql_query(&mysql_conn, sql) != 0) {
			write_log("insert logs failed.");
		}
	}
	task_mysql_close(&mysql_conn);
}

/* 同步配置线程 */
void load_worker() {
//	pthread_detach(pthread_self());
	for (;;) {
		if (server.shutdown) {
			break;
		}
		/* 加载任务到新建列表中 */
		if (strcmp(server.run_type, "file") == 0) {
			load_file_tasks(server.task_file);
		} else if (strcmp(server.run_type, "mysql") == 0) {
			load_mysql_tasks();
		}
		usleep(SYNC_CONFIG_TIME);
	}
}

/* 文件配置计划任务加载 */
void load_file_tasks(const char *task_file) {
	FILE *fp;
	fp = fopen(server.task_file, "r");
	if (NULL == fp) {
		write_log("open config file faild.");
	}
	char line[BUFSIZE] = { 0x00 };

	pthread_mutex_lock(&LOCK_task);
	/* 初始化任务列表 */
	task_init(task_list);

	while (NULL != fgets(line, BUFSIZE, fp)) {
		/* 忽略空行和＃号开头的行 */
		if ('\n' == line[0] || '#' == line[0]) {
			continue;
		}
		line[strlen(line) + 1] = '\0';
		struct tm _stime, _etime;

		/* 创建并初始化一个新节点 */
		s_task_item *taskItem;
		taskItem = (s_task_item *) malloc(sizeof(s_task_item));

		taskItem->next = NULL;
		taskItem->prev = NULL;
		taskItem->runTimes = 0;

		/* 按格式读取 */
		sscanf(
				line,
				"%04d-%02d-%02d %02d:%02d:%02d,%04d-%02d-%02d %02d:%02d:%02d,%d,%d,%[^\n]",
				&_stime.tm_year, &_stime.tm_mon, &_stime.tm_mday,
				&_stime.tm_hour, &_stime.tm_min, &_stime.tm_sec,
				&_etime.tm_year, &_etime.tm_mon, &_etime.tm_mday,
				&_etime.tm_hour, &_etime.tm_min, &_etime.tm_sec,
				&taskItem->frequency, &taskItem->times, taskItem->command);

		/* 转化为时间戳 */
		_stime.tm_year -= 1900;
		_etime.tm_year -= 1900;
		_stime.tm_mon -= 1;
		_etime.tm_mon -= 1;

		taskItem->frequency = taskItem->frequency * TIME_UNIT;

		taskItem->startTime = my_mktime(&_stime);
		taskItem->endTime = my_mktime(&_etime);

		time_t nowTime = GetNowTime();

		/* 如果已经结束直接下一个 */
		if (taskItem->endTime <= nowTime || taskItem->nextTime
				> taskItem->endTime) {
			item_free(taskItem, task_list);
			continue;
		}

		/* 计算下次运行点 */
		int step = ceil((nowTime - taskItem->startTime) / taskItem->frequency);
		taskItem->nextTime = taskItem->startTime + ((step + 1)
				* taskItem->frequency);
		while (taskItem->nextTime <= nowTime) {
			taskItem->nextTime += taskItem->frequency;
		}
		/* 更新到任务链表 */
		task_update(taskItem, task_list);
	}
	write_log("load tasks form file.");
	pthread_mutex_unlock(&LOCK_task);
	fclose(fp);
}

void load_mysql_tasks() {
	MYSQL mysql_conn;
	MYSQL_RES *mysql_result;
	MYSQL_ROW mysql_row;
	int row, row_num;

	if (task_mysql_connect(&mysql_conn)) {
		/* 查询结果 */
		if (mysql_query(&mysql_conn, "SELECT * FROM mk_timeproc") != 0) {
			write_log("mysql_query() failed.");
		}
		/* 获取结果集和条数 */
		mysql_result = mysql_store_result(&mysql_conn);
		row_num = mysql_num_rows(mysql_result);

		pthread_mutex_lock(&LOCK_task);
		/* 初始化任务列表 */
		task_init(task_list);

		for (row = 0; row < row_num; row++) {
			mysql_row = mysql_fetch_row(mysql_result);

			struct tm _stime, _etime;
			s_task_item *taskItem;
			taskItem = (s_task_item *) malloc(sizeof(s_task_item));

			taskItem->mail = false;
			taskItem->next = NULL;
			taskItem->prev = NULL;
			taskItem->runTimes = 0;

			/* 格式化开始时间 */
			sscanf(mysql_row[1], "%04d-%02d-%02d %02d:%02d:%02d", &_stime.tm_year,
					&_stime.tm_mon, &_stime.tm_mday, &_stime.tm_hour,
					&_stime.tm_min, &_stime.tm_sec);

			/* 格式化结束时间 */
			sscanf(mysql_row[2], "%04d-%02d-%02d %02d:%02d:%02d", &_etime.tm_year,
					&_etime.tm_mon, &_etime.tm_mday, &_etime.tm_hour,
					&_etime.tm_min, &_etime.tm_sec);
			sprintf(taskItem->command, "%s", mysql_row[5]);

			taskItem->times = 0;
			taskItem->frequency = atoi(mysql_row[3]) * TIME_UNIT;
			taskItem->task_id = atoi(mysql_row[0]);
			taskItem->timeout = atoi(mysql_row[7]);

			/* 转化为时间戳 */
			_stime.tm_year -= 1900;
			_etime.tm_year -= 1900;
			_stime.tm_mon -= 1;
			_etime.tm_mon -= 1;

			taskItem->startTime = my_mktime(&_stime);
			taskItem->endTime = my_mktime(&_etime);

			time_t nowTime = GetNowTime();

			/* 计算下次运行点 */
			int step = ceil((nowTime - taskItem->startTime) / taskItem->frequency);
			taskItem->nextTime = taskItem->startTime + ((step + 1)
					* taskItem->frequency);

			while (taskItem->nextTime <= nowTime) {
				taskItem->nextTime += taskItem->frequency;
			}

			/* 如果已经结束直接下一个 */
			if (taskItem->endTime <= nowTime || taskItem->nextTime
					> taskItem->endTime) {
				continue;
			}
			/* 更新到任务链表 */
			task_update(taskItem, task_list);
		}
		write_log("load %d tasks from mysql.", task_list->count);

		pthread_mutex_unlock(&LOCK_task);
		/* 释放结果集 */
		mysql_free_result(mysql_result);
	}
	task_mysql_close(&mysql_conn);
}
/* 获取配置文件中字符串配置 */
char *get_config_string(char * dest, char *area, char *key){
	return string_copy(dest, c_get_string(area, key, g_config_file));
}
/* 获取配置文件中数字配置 */
int get_config_int(char * area, char * key){
	return c_get_int(area, key, g_config_file);
}
/* 初始化全局通用变量 */
void init_global_params() {
	server.run_type = get_config_string(server.run_type, "main", "run");
	if (strcmp(server.run_type, "file") != 0
			&& strcmp(server.run_type, "mysql") != 0) {
		print_error("error run type : %s, it's is mysql or file\n",
				server.run_type);
		exit(0);
	}

	server.notice = get_config_string(server.notice, "main", "notice");
	if (strcmp(server.notice, "on") != 0 && strcmp(server.notice, "off") != 0) {
		print_error("error notice value : %s, it's is on or off\n", server.notice);
		exit(0);
	}

	server.max_threads = get_config_int("main", "max_threads");
	server.max_threads = 1;
	server.shutdown = 0;
	server.mail_count = 5;
}
/* 释放全局通用变量 */
void free_global_params(){
	free(server.run_type);
	free(server.task_file);
	free(server.notice);
}
/* 初始化mysql相关变量 */
void init_mysql_params() {
	if (strcmp(server.run_type, "file") == 0) {
		server.task_file = get_config_string(server.task_file, "file", "file");
		if (NULL == server.task_file || -1 == access(server.task_file, F_OK)) {
			print_error("task file is not exist.\n");
			exit(0);
		}

	} else if (strcmp(server.run_type, "mysql") == 0) {
		g_mysql_params.host = get_config_string(g_mysql_params.host, "mysql", "host");
		g_mysql_params.username = get_config_string(g_mysql_params.username, "mysql", "username");
		g_mysql_params.passwd = get_config_string(g_mysql_params.passwd, "mysql", "passwd");
		g_mysql_params.dbname = get_config_string(g_mysql_params.dbname, "mysql", "dbname");
		g_mysql_params.port = get_config_int("mysql", "g_mysql_port");
	}
}
/* 释放mysql相关变量 */
void free_mysql_params(){
	free(g_mysql_params.host);
	free(g_mysql_params.username);
	free(g_mysql_params.passwd);
	free(g_mysql_params.dbname);
}
/* 初始化邮件相关变量 */
void init_mail_params() {
	g_mail_params.server = get_config_string(g_mail_params.server, "mail", "server");
	g_mail_params.user = get_config_string(g_mail_params.user, "mail", "user");
	g_mail_params.passwd = get_config_string(g_mail_params.passwd, "mail", "passwd");
	g_mail_params.to = get_config_string(g_mail_params.to, "mail", "to");
	g_mail_params.port = get_config_int("mail", "port");
}
/* 释放邮件相关变量 */
void free_mail_params() {
	free(g_mail_params.server);
	free(g_mail_params.user);
	free(g_mail_params.passwd);
	free(g_mail_params.to);
}

void free_resource() {
	/* 删除PID */
	if (0 == access(PIDFILE, F_OK)) {
		remove(PIDFILE);
	}
	/* 销毁全局变量 */
	if (NULL != task_list) {
		task_free(task_list);
		task_list = NULL;
	}
	if (NULL != g_config_file) {
		free(g_config_file);
		g_config_file = NULL;
	}
	curl_global_cleanup();
	free_global_params();
	free_mysql_params();
	free_mail_params();
}

/* 任务处理 */
void deal_task() {
//	pthread_detach(pthread_self());
	struct s_right_task * taskItem;
	for (;;) {
		if (server.shutdown) {
			break;
		}
		pthread_mutex_lock(&LOCK_right_task);
		while (l_right_task == NULL) {
			pthread_cond_wait(&COND_right_task, &LOCK_right_task);
		}
		taskItem = l_right_task;
		l_right_task = taskItem->next;
		pthread_mutex_unlock(&LOCK_right_task);
		s_task_item * task_item = (s_task_item *) taskItem->item;
		curl_request(task_item);
//		shell_command(task_item);
		free(taskItem);
		usleep(TASK_STEP);
	}
}

/* 创建线程 */
void create_threads() {
	unsigned long i = 0;
	pthread_t tid[server.max_threads];
	pthread_t task_tid, config_tid, mail_tid;
	/* 定时加载配置线程 */
	pthread_create(&config_tid, NULL, (void *) load_worker, NULL);
	/* 邮件队列线程 */
	pthread_create(&mail_tid, NULL, (void *) mail_worker, NULL);
	/* 任务分配线程 */
	pthread_create(&task_tid, NULL, (void *) task_worker, NULL);
	/* 创建即时任务线程 */
	for (i = 0; i < server.max_threads; i++) {
		pthread_create(&tid[i], NULL, (void *) deal_task, (void *) i);
	}
	pthread_join(task_tid, NULL);
	pthread_join(config_tid, NULL);
	pthread_join(mail_tid, NULL);
	for (i = 0; i < server.max_threads; i++) {
		pthread_join(tid[i], NULL);
	}
}
/* 父进程信号处理 */
void signal_master(const int signal) {
	write_log("kill master with signal %d.", signal);
	/* 给进程组发送SIGTERM信号，结束子进程 */
	server.shutdown = 1;
	free_resource();
	kill(0, SIGTERM);
	exit(0);
}

/* 子进程信号处理 */
void signal_worker(const int signal) {
	free_resource();
	write_log("kill worker with signal %d.", signal);
	exit(0);
}

/* 守护子进程 */
void create_child(void) {
	/* 派生子进程（工作进程） */
	pid_t worker_pid_wait;
	pid_t worker_pid = fork();

	/* 如果派生进程失败，则退出程序 */
	if (worker_pid < 0) {
		print_error("error: %s:%d\n", __FILE__, __LINE__);
		exit(0);
	}
	/* 父进程内容 */
	if (worker_pid > 0) {

		signal(SIGPIPE, SIG_IGN);
		signal(SIGCHLD, SIG_IGN);
		signal(SIGTTOU, SIG_IGN);
		signal(SIGHUP, SIG_IGN);

		/* 处理kill信号 */
		signal(SIGINT, signal_master);
		signal(SIGKILL, signal_master);
		signal(SIGQUIT, signal_master);
		signal(SIGTERM, signal_master);

		/* 处理段错误信号 */
		signal(SIGSEGV, signal_master);

		/* 如果子进程终止，则重新派生新的子进程 */
		while (1) {
			worker_pid_wait = wait(NULL);
			if (worker_pid_wait < 0) {
				write_log("restart worker.");
				worker_pid = fork();
				if (worker_pid == 0) {
					break;
				}
			}
			usleep(100000);
		}
	}
	/** 子进程处理 **/
	/* 忽略Broken Pipe信号 */
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	/* 处理kill信号 */
	signal(SIGINT, signal_worker);
	signal(SIGKILL, signal_worker);
	signal(SIGQUIT, signal_worker);
	signal(SIGTERM, signal_worker);
	/* 处理段错误信号 */
	signal(SIGSEGV, signal_worker);
}

/* 进程ID */
void create_pid_file(void) {
	FILE *fp_pidfile;
	fp_pidfile = fopen(PIDFILE, "w");
	fprintf(fp_pidfile, "%d\n", getpid());
	fclose(fp_pidfile);
}

/* 守护进程 */
void daemonize(void) {
	pid_t pid = fork();
	if (pid < 0) {
		print_error("fork() failed : %d\n", pid);
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

void config_testing(){
	/* mysql配置尝试连接 */
	MYSQL mysql_conn;
	if (!task_mysql_connect(&mysql_conn)) {
		print_error("task_mysql_connect() failed, please check mysql config.\n");
		task_mysql_close(&mysql_conn);
		exit(1);
	}
	task_mysql_close(&mysql_conn);
	/* 邮件配置检测 */
	/*int send = send_notice_mail("taskserver test mail.", "taskserver test mail.");
	if (send) {
		print_error("send mail failed, please check mail config:%d.\n", send);
		exit(1);
	}*/
}

/* 主模块 */
int main(int argc, char *argv[], char *envp[]) {
	bool daemon = false;
	/* 输入参数格式定义 */
	struct option long_options[] = { { "daemon", 0, 0, 'd' }, { "config", 1, 0,
			'c' }, { "version", 0, 0, 'v' }, { "help", 0, 0, 'h' }, { NULL, 0,
			0, 0 } };

	if (1 == argc) {
		print_error("please use %s --help\n", argv[0]);
		exit(1);
	}

	/* 读取参数 */
	int c;
	while ((c = getopt_long(argc, argv, "v:t:c:dh", long_options, NULL)) != -1) {
		switch (c) {
		case 'd':
			daemon = true;
			break;
		case 'c':
			g_config_file = malloc(strlen(optarg) + 1);
			sprintf(g_config_file, "%s", optarg);
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
	/* 参数判断 */
	if (optind != argc) {
		print_error("too many arguments\nTry `%s --help' for more information.\n", argv[0]);
		exit(0);
	}
	init_global_params();
	init_mysql_params();
	if (strcmp(server.notice, "on") == 0) {
		init_mail_params();
	}
	config_testing();
	if (daemon == true) {
		daemonize();
	}
	create_pid_file();
	/* 初始化任务列表 */
	task_list = (l_task_list *) malloc(sizeof(l_task_list));
	if (NULL == task_list) {
		write_log("tasklist malloc failed.");
	};
	task_init(task_list);
	create_child();
	curl_global_init(CURL_GLOBAL_ALL);
	create_threads();
	free_resource();
	return 0;
}
