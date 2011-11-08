/* 
 * File:   main.c
 * Author: raink.kid@gmail.com
 * Created on 2010年12月20日, 上午9:14
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
#include "task.h"

#define PIDFILE  "./taskCron.pid"
#define VERSION  "1.0"
#define HELPINFO "Cron author raink.kid@gmail.com\n" \
                 "-h, --help     display this message then exit.\n" \
                 "-v, --version  display version information then exit.\n" \
                 "-l, --loglevel @TODO.\n" \
                 "-c, --config <path>  read crontab task config.\n" \
                 "-d, --daemon   run as a daemon.\n"

static void ExitHandle(const int signal);
static void ReadTaskConfig(const char *taskConfigFile);
static time_t GetNowTime();

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
    struct option long_options[] = {
        {"daemon",   0, 0, 'd' },
        {"help",     0, 0, 'h' },
        {"version",  0, 0, 'v' },
        {"config",   1, 0, 'c' },
        {"loglevel", 0, 0, 'l' },
        {0, 0, 0, 0 }
    };

    if (1 == argc) {
        fprintf(stderr, "Please use %s --help\n", argv[0]);
        exit(1);
    }

    //读取参数
    char c;
    while ((c = getopt_long(argc, argv, "dhvc:l", long_options, NULL)) != -1) {
        switch (c) {
            case 'd' : 
				daemon = 1; 
				break;
            case 'c' : 
				taskConfigFile = strdup(optarg); 
				break;
            case 'l' : 
				logLevel = atoi(optarg); 
				break;
            case 'v' : 
				printf("%s\n\n", VERSION); exit(EXIT_SUCCESS); 
				break;
            case -1  : 
				break;
            case 'h' :
            default  : 
				printf("%s\n", HELPINFO); exit(EXIT_SUCCESS); 
				break;
        }
    }

    //参数判断
    if (optind != argc) {
        fprintf(stderr,  "Too many arguments\nTry `%s --help' for more information.\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    //读取任务配置
    if (NULL == taskConfigFile || -1 == access(taskConfigFile, F_OK)) {
        fprintf(stderr, "Please use task config: -c <path> or --config <path>\n\n");
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
        for(i=0; i < NOFILE; i++) {
            close(i);
        }
    }

    //进程PID
    FILE *fp_pidfile;
    fp_pidfile = fopen(PIDFILE, "w");
    fprintf(fp_pidfile, "%d\n", getpid());
    fclose(fp_pidfile);

    //信号处理
    signal(SIGINT, ExitHandle);
    signal(SIGKILL, ExitHandle);
    signal(SIGQUIT, ExitHandle);
    signal(SIGTERM, ExitHandle);
    signal(SIGHUP, ExitHandle);

    //内存分配
    taskList = (TaskList *)malloc(sizeof(TaskList));
    assert(NULL != taskList);
    taskList->count = 0;
    taskList->head = NULL;
    taskList->tail = NULL;

    //读取任务配置
    ReadTaskConfig(taskConfigFile);
    
    printf("head=%p,tail=%p\n", taskList->head, taskList->tail);
    
    TaskItem *temp_item = taskList->head;
    while(NULL != temp_item) {
        printf("prev=%p, next=%p,self=%p, starTime=%ld,endTime=%ld,nextTime=%ld,times=%d,fre=%d,command=%s\n",
                temp_item->prev,temp_item->next, temp_item,
                temp_item->startTime,temp_item->endTime,temp_item->nextTime,temp_item->times,temp_item->frequency, temp_item->command);
        temp_item = temp_item->next;
    }
    
    //开始处理
    int delay = 1;

    TaskItem *temp, *temp_next;
    while (NULL != taskList->head) {
        time_t nowTime = GetNowTime();
        while(NULL != (temp = taskList->head)) {
            //临时记录
            temp_next = temp->next;
            //大于当前时间跳出
            if (nowTime < temp->nextTime) {
                delay = temp->nextTime - nowTime;
                break;
            }
            //执行
            system(temp->command);
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
                }
                else {
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
            /*
            TaskItem *temp_item = taskList->head;
            while(NULL != temp_item) {
                printf("prev=%p, next=%p,self=%p, starTime=%ld,endTime=%ld,nextTime=%ld,times=%d,fre=%d,command=%s\n",
                        temp_item->prev,temp_item->next, temp_item,
                        temp_item->startTime,temp_item->endTime,temp_item->nextTime,temp_item->times,temp_item->frequency, temp_item->command);
                temp_item = temp_item->next;
            }
             */
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
    if(0 == access(PIDFILE, F_OK)) {
        remove(PIDFILE);
    }

    exit(EXIT_SUCCESS);
}

/**
 * 任务计划配置读取
 * @param taskConfigFile
 */
static void ReadTaskConfig(const char *taskConfigFile) {
    FILE *fp;
    fp=fopen(taskConfigFile, "r");
    assert(NULL != fp);

    char line[BUFSIZ];
    
    while (NULL != (fgets(line, BUFSIZ, fp))) {
        if ('\n' == line[0] || '#' == line[0]) {
            continue;
        }
        line[strlen(line)-1] = '\0';

        //开始,结束时间
        struct tm _stime, _etime;

        TaskItem *taskItem;
        taskItem = (TaskItem *)malloc(sizeof(TaskItem));

        assert(NULL != taskItem);
        taskItem->next = NULL;
        taskItem->prev = NULL;
        taskItem->runTimes = 0;
        
        //按格式读取
        sscanf(line, "%04d-%02d-%02d %02d:%02d:%02d,%04d-%02d-%02d %02d:%02d:%02d,%d,%d,%[^\n]",
                &_stime.tm_year, &_stime.tm_mon, &_stime.tm_mday, &_stime.tm_hour, &_stime.tm_min, &_stime.tm_sec,
                &_etime.tm_year, &_etime.tm_mon, &_etime.tm_mday, &_etime.tm_hour, &_etime.tm_min, &_etime.tm_sec,
                &taskItem->frequency,
                &taskItem->times,
                taskItem->command);

        //转化为时间戳
        _stime.tm_year -= 1900;
        _etime.tm_year -= 1900;
        _stime.tm_mon -= 1;
        _etime.tm_mon -= 1;

		fprintf(stderr, "%s\n", taskItem->command);
        taskItem->startTime = mktime(&_stime);
        taskItem->endTime = mktime(&_etime);
        //当前时间
        time_t nowTime = GetNowTime();
        //计算下次运行点
        taskItem->nextTime = taskItem->startTime + taskItem->frequency;
        while(taskItem->nextTime <= nowTime) {
            taskItem->nextTime += taskItem->frequency;
        }
        //是否结束
        if (taskItem->endTime <= nowTime || taskItem->nextTime > taskItem->endTime) {
            continue;
        }
        
        //printf("nowTime=%ld,starTime=%ld,endTime=%ld,nextTime=%ld,frequency=%d,times=%d,command=%s\n",nowTime,taskItem->startTime, taskItem->endTime,taskItem->nextTime,taskItem->frequency, taskItem->times, taskItem->command);

        Update(taskItem, taskList);
    }
    
    fclose(fp);
}

/**
 * 获取当前时间戳
 * @return
 */
static time_t GetNowTime() {
    time_t nowTime;
    time(&nowTime);
    return nowTime;
}

/**
 * 信号处理
 * @param signal
 */
static void ExitHandle(const int signal) {
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
    if(0 == access(PIDFILE, F_OK)) {
        remove(PIDFILE);
    }
    
    exit(EXIT_SUCCESS);
}


