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
    int task_id;
    time_t startTime;
    time_t endTime;
    time_t nextTime;
    int  frequency;
    int timeout;
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
void task_init(TaskList *);

//添加节点到尾部
void task_add(TaskList *, TaskItem *);

//更新节点
void task_update(TaskItem *, TaskList *);

//任务列表是否空
bool task_isempty(const TaskList *);

//任务列表销毁
void task_free(TaskList *);

//任务节点销毁
void item_free(TaskItem *task_item, TaskList *task_list);

#ifdef	__cplusplus
}
#endif

#endif	/* TASK_H */

