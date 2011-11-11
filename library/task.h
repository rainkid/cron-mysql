/* 
 * File:   list.h
 * Author: venkman
 *
 * Created on 2010年12月20日, 上午9:21
 */

#ifndef TASK_H
#define	TASK_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <sys/types.h>
#include <stdbool.h>
    
//ITEM
typedef struct _taskItem {
    time_t startTime;
    time_t endTime;
    time_t nextTime;
    int  frequency;
    int times;
    int runTimes;
    char command[BUFSIZ];
    struct _taskItem *next;
    struct _taskItem *prev;
} TaskItem;

//LIST
typedef struct _taskList {
    //头
    TaskItem *head;
    //尾
    TaskItem *tail;
    //任务节点数量
    int count;
} TaskList;

//任务初始化
bool Task_Init(TaskList *);

//添加节点到尾部
bool Task_Add(TaskList *, TaskItem *);

//更新节点
bool Task_Update(TaskItem *, TaskList *);

//任务列表是否空
bool Task_IsEmpty(const TaskList *);

//任务列表销毁
bool Task_Free(TaskList *);

//任务节点销毁
bool Item_Free(TaskItem *);

#ifdef	__cplusplus
}
#endif

#endif	/* TASK_H */

