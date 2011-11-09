/* 
 * File:   main.c
 * Author: raink.kid@gmail.com
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/param.h>
#include <assert.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>

#include <mysql/mysql.h>

#include "lib/task.h"
#include "lib/tool.h"

#define PIDFILE  "./cron.pid"
#define VERSION  "1.0"
#define HELPINFO "author raink.kid@gmail.com\n" \
                 "-h, --help     display this message then exit.\n" \
                 "-v, --version  display version information then exit.\n" \
                 "-l, --loglevel @TODO.\n" \
                 "-c, --config <path>  read crontab task config.\n" \
                 "-d, --daemon   run as a daemon.\n"

static void Cron_ExitHandle(const int signal);
void Cron_File_Config(const char *taskConfigFile);
void Cron_Mysql_Config();

//任务节点
TaskList *taskList = NULL;
//任务配置路径
char *taskConfigFile = NULL;
//日志
int logLevel = 0;

/*
 * 主模块
 */
int main(int argc, char** argv) {
	int daemon = 0;
	struct option long_options[] = { { "daemon", 0, 0, 'd' }, { "help", 0, 0,
			'h' }, { "version", 0, 0, 'v' }, { "config", 1, 0, 'c' }, {
			"loglevel", 0, 0, 'l' }, { 0, 0, 0, 0 } };

	if (1 == argc) {
		fprintf(stderr, "Please use %s --help\n", argv[0]);
		exit(1);
	}

	//读取参数
	char c;
	while ((c = getopt_long(argc, argv, "dhvc:l", long_options, NULL)) != -1) {
		switch (c) {
		case 'd':
			daemon = 1;
			break;
		case 'c':
			taskConfigFile = strdup(optarg);
			break;
		case 'l':
			logLevel = atoi(optarg);
			break;
		case 'v':
			printf("%s\n\n", VERSION);
			exit(EXIT_SUCCESS);
			break;
		case -1:
			break;
		case 'h':
		default:
			printf("%s\n", HELPINFO);
			exit(EXIT_SUCCESS);
			break;
		}
	}

	//参数判断
	if (optind != argc) {
		fprintf(stderr,
				"Too many arguments\nTry `%s --help' for more information.\n",
				argv[0]);
		exit(EXIT_FAILURE);
	}

	//读取任务配置
	if (NULL == taskConfigFile || -1 == access(taskConfigFile, F_OK)) {
		fprintf(stderr,
				"Please use task config: -c <path> or --config <path>\n\n");
		exit(EXIT_FAILURE);
	}

	//守护进程
	if (daemon) {
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

	//进程PID
	FILE *fp_pidfile;
	fp_pidfile = fopen(PIDFILE, "w");
	fprintf(fp_pidfile, "%d\n", getpid());
	fclose(fp_pidfile);

	//信号处理
	signal(SIGINT, Cron_ExitHandle);
	signal(SIGKILL, Cron_ExitHandle);
	signal(SIGQUIT, Cron_ExitHandle);
	signal(SIGTERM, Cron_ExitHandle);
	signal(SIGHUP, Cron_ExitHandle);

	//内存分配
	taskList = (TaskList *) malloc(sizeof(TaskList));
	assert(NULL != taskList);
	taskList->count = 0;
	taskList->head = NULL;
	taskList->tail = NULL;

	//读取任务配置
	//    Cron_File_Config(taskConfigFile);
	Cron_Mysql_Config();

	/*fprintf(stderr, "head=%p,tail=%p\n", taskList->head, taskList->tail);

	TaskItem *temp_item = taskList->head;
	while (NULL != temp_item) {
		fprintf(
				stderr,
				"prev=%p, next=%p,self=%p, starTime=%ld,endTime=%ld,nextTime=%ld,times=%d,fre=%d,command=%s\n",
				temp_item->prev, temp_item->next, temp_item,
				temp_item->startTime, temp_item->endTime, temp_item->nextTime,
				temp_item->times, temp_item->frequency, temp_item->command);
		temp_item = temp_item->next;
	}*/

	//开始处理
	int delay = 1;

	TaskItem *temp;
	while (NULL != taskList->head) {
		time_t nowTime = GetNowTime();
		while (NULL != (temp = taskList->head)) {
			 /*fprintf(stderr, "prev=%p, next=%p,self=%p, starTime=%ld,endTime=%ld,nextTime=%ld,times=%d,fre=%d,command=%s\n",
					 temp->prev,temp->next, temp,
					 temp->startTime,temp->endTime,temp->nextTime,temp->times,temp->frequency, temp->command);*/
			//大于当前时间跳出
			if (nowTime < temp->nextTime) {
				delay = temp->nextTime - nowTime;
				break;
			}
			//执行
			//system(temp->command);
			Cron_Curl(temp->command);
			(temp->runTimes)++;
			if (temp->next) {
				//删除头部
				taskList->head = temp->next;
				temp->next->prev = NULL;
				//尾巴
				if (temp == taskList->tail) {
					taskList->tail = temp->prev;
				}
				//添加
				if (0 == temp->times || temp->runTimes < temp->times) {
					temp->prev = NULL;
					temp->next = NULL;
					temp->nextTime = temp->frequency + nowTime;
					//重新添加
					Update(temp, taskList);
				} else {
					free(temp);
					temp = NULL;
				}
			} else {
				if (temp->times > 0 && temp->runTimes > temp->times) {
					free(temp);
					temp = NULL;
					taskList->head = NULL;
					taskList->tail = NULL;
					free(taskList);
					taskList = NULL;
					delay = 1;
				} else {
					temp->nextTime = temp->frequency + nowTime;
					delay = temp->frequency;
				}
				break;
			}
		}

		if (taskList) {
			/*TaskItem *temp_item = taskList->head;
			 while(NULL != temp_item) {
			 temp_item = temp_item->next;
			 }*/
			sleep(delay);
		} else {
			break;
		}
	}

	//结束清理
	//销毁配置文件内存分配
	if (NULL != taskConfigFile) {
		free(taskConfigFile);
		taskConfigFile = NULL;
	}
	//销毁任务
	if (NULL != taskList) {
		TaskListFree(taskList);
		taskList = NULL;
	}
	//删除PID
	if (0 == access(PIDFILE, F_OK)) {
		remove(PIDFILE);
	}

	exit(EXIT_SUCCESS);
}

/**
 * 信号处理
 */
static void Cron_ExitHandle(const int signal) {
	//销毁配置文件内存分配
	if (NULL != taskConfigFile) {
		free(taskConfigFile);
		taskConfigFile = NULL;
	}
	//销毁任务
	if (NULL != taskList) {
		TaskListFree(taskList);
		taskList = NULL;
	}
	//删除PID
	if (0 == access(PIDFILE, F_OK)) {
		remove(PIDFILE);
	}

	exit(EXIT_SUCCESS);
}

void Cron_File_Config(const char *taskConfigFile) {
	FILE *fp;
	fp = fopen(taskConfigFile, "r");
	assert(NULL != fp);

	char line[BUFSIZ] = { 0x00 };
	while (NULL != (fgets(line, BUFSIZ, fp))) {
		if ('\n' == line[0] || '#' == line[0]) {
			continue;
		}
		line[strlen(line) - 1] = '\0';

		//开始,结束时间
		struct tm _stime, _etime;

		TaskItem *taskItem;
		taskItem = (TaskItem *) malloc(sizeof(TaskItem));

		assert(NULL != taskItem);
		taskItem->next = NULL;
		taskItem->prev = NULL;
		taskItem->runTimes = 0;

		//按格式读取
		sscanf(
				line,
				"%04d-%02d-%02d %02d:%02d:%02d,%04d-%02d-%02d %02d:%02d:%02d,%d,%d,%[^\n]",
				&_stime.tm_year, &_stime.tm_mon, &_stime.tm_mday,
				&_stime.tm_hour, &_stime.tm_min, &_stime.tm_sec,
				&_etime.tm_year, &_etime.tm_mon, &_etime.tm_mday,
				&_etime.tm_hour, &_etime.tm_min, &_etime.tm_sec,
				&taskItem->frequency, &taskItem->times, taskItem->command);

		//转化为时间戳
		_stime.tm_year -= 1900;
		_etime.tm_year -= 1900;
		_stime.tm_mon -= 1;
		_etime.tm_mon -= 1;

		taskItem->startTime = mktime(&_stime);
		taskItem->endTime = mktime(&_etime);
		//当前时间
		time_t nowTime = GetNowTime();
		//计算下次运行点
		taskItem->nextTime = taskItem->startTime + taskItem->frequency;
		while (taskItem->nextTime <= nowTime) {
			taskItem->nextTime += taskItem->frequency;
		}
		//是否结束
		if (taskItem->endTime <= nowTime || taskItem->nextTime
				> taskItem->endTime) {
			continue;
		}
		Update(taskItem, taskList);
	}
	fclose(fp);
}

void Cron_Mysql_Config() {
	char * host, *user, *passwd, *db/*, *port*/;

	host = strdup("127.0.0.1");
	user = strdup("root");
	passwd = strdup("root");
	db = strdup("test");

	MYSQL mysql_conn;
	MYSQL_RES *mysql_result;
	MYSQL_ROW mysql_row;
	char sql[BUFSIZ] = { 0x00 };
	int row, row_num;

	sprintf(sql, "%s", "SELECT * FROM mk_timeproc");
	if (mysql_init(&mysql_conn) == NULL) {
		fprintf(stderr, "%s\n", "Initialization fails.");
		mysql_close(&mysql_conn);
		return;
	}

	if (mysql_real_connect(&mysql_conn, host, user, passwd, db, 3306, NULL, 128)
			== NULL) {
		fprintf(stderr, "%s\n", "Connection fails.");
		mysql_close(&mysql_conn);
		return;
	}

	if (mysql_query(&mysql_conn, "SELECT * FROM mk_timeproc") != 0) {
		fprintf(stderr, "%s\n", "Query fails.");
		mysql_close(&mysql_conn);
		return;
	}

	mysql_result = mysql_store_result(&mysql_conn);
	row_num = mysql_num_rows(mysql_result);

	char start_time[BUFSIZ] = { 0x00 };
	char end_time[BUFSIZ] = { 0x00 };
	char command[BUFSIZ] = { 0x00 };
	//	char times[BUFSIZ] = { 0x00 };

	for (row = 0; row < row_num; row++) {
		mysql_row = mysql_fetch_row(mysql_result);
		//开始,结束时间
		struct tm _stime, _etime;

		TaskItem *taskItem;
		taskItem = (TaskItem *) malloc(sizeof(TaskItem));

		assert(NULL != taskItem);
		taskItem->next = NULL;
		taskItem->prev = NULL;
		taskItem->runTimes = 0;
		//按格式读取
		sprintf(start_time, "%s", mysql_row[1]);
		sscanf(start_time, "%04d-%02d-%02d %02d:%02d:%02d", &_stime.tm_year,
				&_stime.tm_mon, &_stime.tm_mday, &_stime.tm_hour,
				&_stime.tm_min, &_stime.tm_sec);
		sprintf(end_time, "%s", mysql_row[2]);
		sscanf(end_time, "%04d-%02d-%02d %02d:%02d:%02d", &_etime.tm_year,
				&_etime.tm_mon, &_etime.tm_mday, &_etime.tm_hour,
				&_etime.tm_min, &_etime.tm_sec);
		sprintf(command, "%s", mysql_row[5]);
		sprintf(taskItem->command, "%s", command);
		taskItem->frequency = 5;
		taskItem->times = 100;

		//转化为时间戳
		_stime.tm_year -= 1900;
		_etime.tm_year -= 1900;
		_stime.tm_mon -= 1;
		_etime.tm_mon -= 1;

		taskItem->startTime = mktime(&_stime);
		taskItem->endTime = mktime(&_etime);
		//当前时间
		time_t nowTime = GetNowTime();
		//计算下次运行点
		taskItem->nextTime = taskItem->startTime + taskItem->frequency;
		while (taskItem->nextTime <= nowTime) {
			taskItem->nextTime += taskItem->frequency;
		}
		/*fprintf(stderr, "prev=%p, next=%p,self=%p, starTime=%ld,endTime=%ld,nextTime=%ld,times=%d,fre=%d,command=%s\n",
		 taskItem->prev, taskItem->next, taskItem, taskItem->startTime,
		 taskItem->endTime, taskItem->nextTime, taskItem->times,
		 taskItem->frequency, taskItem->command);*/
		//是否结束
		if (taskItem->endTime <= nowTime || taskItem->nextTime
				> taskItem->endTime) {
			continue;
		}
		Update(taskItem, taskList);
	}

	mysql_free_result(mysql_result);
	mysql_close(&mysql_conn);
}
