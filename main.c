/* 
 * File:   main.c
 * Author: raink.kid@gmail.com
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/param.h>
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
#include <mysql/mysql.h>

#include "library/task.h"
#include "library/tool.h"

#include "library/base64.h"
#include "library/send_mail.h"

#define PIDFILE  "./cron.pid"
#define VERSION  "1.0"

static void kill_signal_master(const int signal);
static void kill_signal_worker(const int signal);
void task_file_load(const char *config_file);
void task_mysql_load();
static void task_worker();
static void config_worker();

/* 任务节点 */
TaskList *taskList = NULL;
/* 任务配置路径 */
char *config_file = NULL;
/* 日志 */
int logLevel = 0;
/* mysql连接 */
MYSQL mysql_conn;
/* 同步配置时间 */
int sync_config_time = 60;

static void usage(){
	printf("author raink.kid@gmail.com\n" \
		 "-h, --help     display this message then exit.\n" \
		 "-v, --version  display version information then exit.\n" \
		 "-l, --loglevel @TODO.\n" \
		 "-c, --config <path>  read crontab task config.\n" \
		 "-d, --daemon   run as a daemon.\n\n");
}

/* 任务处理线程 */
static void task_worker() {
	pthread_detach(pthread_self());

	/* 开始处理 */
	TaskItem *temp;
	while(1){
		if (NULL != taskList->head) {
			time_t nowTime = GetNowTime();
			while (NULL != (temp = taskList->head)) {
				//大于当前时间跳出
				if (nowTime < temp->nextTime) {
					break;
				}
				/*fprintf(
						stderr,
						"prev=%p, next=%p,self=%p, starTime=%ld,endTime=%ld,nextTime=%ld,times=%d,fre=%d,command=%s\n",
						temp->prev, temp->next, temp, temp->startTime,
						temp->endTime, temp->nextTime, temp->times,
						temp->frequency, temp->command);*/
				/* 执行任务 */
				//system(temp->command);
				Curl_Request(temp->command);

				(temp->runTimes)++;
				if (temp->next) {
					/* 删除头部 */
					taskList->head = temp->next;
					temp->next->prev = NULL;
					/* 尾巴 */
					if (temp == taskList->tail) {
						taskList->tail = temp->prev;
					}
					/* 添加 */
					if (0 == temp->times || temp->runTimes < temp->times) {
						temp->prev = NULL;
						temp->next = NULL;
						temp->nextTime = temp->frequency + nowTime;
						/* 重新添加 */
						Task_Update(temp, taskList);
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
					} else {
						temp->nextTime = temp->frequency + nowTime;
					}
					break;
				}
			}

			if (taskList) {
				/*TaskItem *temp_item = taskList->head;
				 while(NULL != temp_item) {
				 temp_item = temp_item->next;
				 }
				sleep(delay);*/
			} else {
				break;
			}
		}
	}
}

/* 同步进程 */
static void config_worker() {
	pthread_detach(pthread_self());
	/*重新内存分配*/
	while(1) {
		taskList->count = 0;
		taskList->head = NULL;
		taskList->tail = NULL;
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
	exit(EXIT_SUCCESS);
}

/*子进程信号处理*/
static void kill_signal_worker(const int signal) {

	/*销毁配置文件内存分配*/
	if (NULL != config_file) {
		free(config_file);
		config_file = NULL;
	}
	/*销毁任务*/
	if (NULL != taskList) {
		Task_Free(taskList);
		taskList = NULL;
	}
	/*关闭mysql连接*/
	mysql_close(&mysql_conn);

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
	while (!feof(fp)) {
		/* 读取文件内容 */
		fgets(line, BUFSIZ, fp);

		/* 忽略空行和＃号开头的行 */
		if ('\n' == line[0] || '#' == line[0]) {
			continue;
		}
		line[strlen(line) + 1] = '\0';
		/* 开始,结束时间 */
		struct tm _stime, _etime;

		/* 创建并初始化一个新节点 */
		TaskItem *taskItem;
		taskItem = (TaskItem *) malloc(sizeof(TaskItem));
		assert(NULL != taskItem);

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

		taskItem->startTime = mktime(&_stime);
		taskItem->endTime = mktime(&_etime);

		/* 当前时间 */
		time_t nowTime = GetNowTime();

		/* 如果已经结束直接下一个 */
		if (taskItem->endTime <= nowTime || taskItem->nextTime > taskItem->endTime) {
			continue;
		}

		/*计算下次运行点*/
		if (taskItem->nextTime == 0) {
			int st =
					ceil((nowTime - taskItem->startTime) / taskItem->frequency);
			taskItem->nextTime = taskItem->startTime + ((st + 1)
					* taskItem->frequency);
		}
		while (taskItem->nextTime <= nowTime) {
			taskItem->nextTime += taskItem->frequency;
		}
//		fprintf(stderr, "prev=%p, next=%p,self=%p, starTime=%ld,endTime=%ld,nextTime=%ld,times=%d,frequency=%d,command=%s\n",
//		 taskItem->prev, taskItem->next, taskItem, taskItem->startTime,
//		 taskItem->endTime, taskItem->nextTime, taskItem->times,
//		 taskItem->frequency, taskItem->command);
		/* 更新到任务链表 */
		Task_Update(taskItem, taskList);
	}
	fclose(fp);
}

/* mysql任务加载 */
void task_mysql_load() {

	MYSQL_RES *mysql_result;
	MYSQL_ROW mysql_row;
	char sql[BUFSIZ] = {0x00};
	int row, row_num;

	char start_time[BUFSIZ] = {0x00};
	char end_time[BUFSIZ] = {0x00};
	char command[BUFSIZ] = {0x00};
	int frequency = 0;
	int times = 0;

	/* 查询sql */
	sprintf(sql, "%s", "SELECT * FROM mk_timeproc");

	/* 查询结果 */
	if (mysql_query(&mysql_conn, sql) != 0) {
		fprintf(stderr, "%s\n", "Mysql Query fails.");
		mysql_close(&mysql_conn);
		return;
	}

	/* 获取结果集和条数 */
	mysql_result = mysql_store_result(&mysql_conn);
	row_num = mysql_num_rows(mysql_result);

	/*创建并初始化一个新节点*/
	TaskItem *taskItem;
	taskItem = (TaskItem *) malloc(sizeof(TaskItem));
	assert(NULL != taskItem);

	taskItem->next = NULL;
	taskItem->prev = NULL;
	taskItem->runTimes = 0;

	/* 取数据 */
	for (row = 0; row < row_num; row++) {
		mysql_row = mysql_fetch_row(mysql_result);

		/* 开始,结束时间 */
		struct tm _stime, _etime;
		TaskItem *taskItem;
		taskItem = (TaskItem *) malloc(sizeof(TaskItem));
		assert(NULL != taskItem);

		taskItem->next = NULL;
		taskItem->prev = NULL;
		taskItem->runTimes = 0;

		/* 格式化开始时间 */
		sprintf(start_time, "%s", mysql_row[1]);
		sscanf(start_time, "%04d-%02d-%02d %02d:%02d:%02d", &_stime.tm_year,
				&_stime.tm_mon, &_stime.tm_mday, &_stime.tm_hour,
				&_stime.tm_min, &_stime.tm_sec);

		/* 格式化结束时间 */
		sprintf(end_time, "%s", mysql_row[2]);
		sscanf(end_time, "%04d-%02d-%02d %02d:%02d:%02d", &_etime.tm_year,
				&_etime.tm_mon, &_etime.tm_mday, &_etime.tm_hour,
				&_etime.tm_min, &_etime.tm_sec);

		/* 格式化命令 */
		sprintf(command, "%s", mysql_row[5]);
		sprintf(taskItem->command, "%s", command);

		frequency = atoi(mysql_row[3]);
		times = atoi(mysql_row[8]);

		taskItem->frequency = frequency;
		taskItem->times = times;

		/* 转化为时间戳 */
		_stime.tm_year -= 1900;
		_etime.tm_year -= 1900;
		_stime.tm_mon -= 1;
		_etime.tm_mon -= 1;

		taskItem->startTime = mktime(&_stime);
		taskItem->endTime = mktime(&_etime);

		/* 当前时间 */
		time_t nowTime = GetNowTime();

		/* 如果已经结束直接下一个 */
		if (taskItem->endTime <= nowTime || taskItem->nextTime > taskItem->endTime) {
			continue;
		}

		/* 计算下次运行点 */
		if (taskItem->nextTime == 0) {
			int perst = ceil((nowTime - taskItem->startTime) / taskItem->frequency);
			taskItem->nextTime = taskItem->startTime + ((perst + 1) * taskItem->frequency);
		}
		while (taskItem->nextTime <= nowTime) {
			taskItem->nextTime += taskItem->frequency;
		}
//		fprintf(
//				stderr,
//				"prev=%p, next=%p,self=%p, starTime=%ld,endTime=%ld,nextTime=%ld,times=%d,fre=%d,command=%s\n",
//				taskItem->prev, taskItem->next, taskItem, taskItem->startTime,
//				taskItem->endTime, taskItem->nextTime, taskItem->times,
//				taskItem->frequency, taskItem->command);
		/* 更新到任务链表 */
		Task_Update(taskItem, taskList);
	}
	/* 释放结果集 */
	mysql_free_result(mysql_result);
}

/* 主模块 */
int main(int argc, char *argv[], char *envp[]) {
	bool daemon = false;

	/* mysql相关参数 */
	char * host, *user, *passwd, *dbname;
	int port;

	host = strdup("127.0.0.1");
	user = strdup("root");
	passwd = strdup("root");
	dbname = strdup("test");
	port = 3306;

	/* 输入参数 */
	struct option long_options[] = {
			{ "daemon", 0, 0, 'd' },
			{ "help", 0, 0,	'h' },
			{ "version", 0, 0, 'v' },
			{ "config", 1, 0, 'c' },
			{"loglevel", 0, 0, 'l' },
			{ 0, 0, 0, 0 } };

	if (1 == argc) {
		fprintf(stderr, "Please use %s --help\n", argv[0]);
		exit(1);
	}

	/* 读取参数 */
	int c;
	while ((c = getopt_long(argc, argv, "dhvc:l", long_options, NULL)) != -1) {
		switch (c) {
		case 'd':
			daemon = true;
			break;
		case 'c':
			config_file = strdup(optarg);
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
			usage();
			exit(EXIT_SUCCESS);
			break;
		}
	}

	/* 参数判断 */
	if (optind != argc) {
		fprintf(stderr,	"Too many arguments\nTry `%s --help' for more information.\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* 读取任务配置 */
	if (NULL == config_file || -1 == access(config_file, F_OK)) {
		fprintf(stderr,
				"Please use task config: -c <path> or --config <path>\n\n");
		exit(EXIT_FAILURE);
	}

	/* 如果加了-d参数，以守护进程运行 */
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
	/* 将进程号写入PID文件 */
	FILE *fp_pidfile;
	fp_pidfile = fopen(PIDFILE, "w");
	fprintf(fp_pidfile, "%d\n", getpid());
	fclose(fp_pidfile);

	/* 派生子进程（工作进程） */
	pid_t worker_pid_wait;
	pid_t worker_pid = fork();

	/* 如果派生进程失败，则退出程序 */
	if (worker_pid < 0) {
		fprintf(stderr, "Error: %s:%d\n", __FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}

	/* 父进程内容 */
	if (worker_pid > 0) {
		/* 处理父进程接收到的kill信号 */

		/* 忽略Broken Pipe信号 */
		signal(SIGPIPE, SIG_IGN);

		/* 处理kill信号 */
		signal(SIGINT, kill_signal_master);
		signal(SIGKILL, kill_signal_master);
		signal(SIGQUIT, kill_signal_master);
		signal(SIGTERM, kill_signal_master);
		signal(SIGHUP, kill_signal_master);

		/* 处理段错误信号 */
		signal(SIGSEGV, kill_signal_master);

		/* 如果子进程终止，则重新派生新的子进程 */
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

	/*****************************************子进程处理************************************/

	/* 忽略Broken Pipe信号 */
	signal(SIGPIPE, SIG_IGN);

	/* 处理kill信号 */
	signal(SIGINT, kill_signal_worker);
	signal(SIGKILL, kill_signal_worker);
	signal(SIGQUIT, kill_signal_worker);
	signal(SIGTERM, kill_signal_worker);
	signal(SIGHUP, kill_signal_worker);

	/* 处理段错误信号 */
	signal(SIGSEGV, kill_signal_worker);

	/* mysql 初始化连接 */
	if (mysql_init(&mysql_conn) == NULL) {
		fprintf(stderr, "%s\n", "Mysql Initialization fails.");
		mysql_close(&mysql_conn);
	}

	/* mysql连接 */
	if (mysql_real_connect(&mysql_conn, host, user, passwd, dbname, port, NULL, 128)== NULL) {
		fprintf(stderr, "%s\n", "Mysql Connection fails.");
		mysql_close(&mysql_conn);
	}
	/* 初始化任务列表 */
	taskList = (TaskList *) malloc(sizeof(TaskList));
	assert(NULL != taskList);

	/* 创建定时同步线程，定时加载配置 */
	pthread_t config_tid;
	pthread_create(&config_tid, NULL, (void *) config_worker, NULL);

	/* 创建任务执行进程 */
	pthread_t task_tid;
	pthread_create(&task_tid, NULL, (void *) task_worker, NULL);

	for(;;);
}
