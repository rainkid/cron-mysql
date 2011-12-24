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
    bool mail;
    char command[BUFSIZ];
    int status;
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

void task_init(TaskList *task_list);
void task_add(TaskList *task_list, TaskItem *task_item);
void task_update(TaskItem *, TaskList *task_list);
bool task_isempty(const TaskList *task_list);
void task_free(TaskList *task_list);
void item_free(TaskItem *task_item, TaskList *task_list);

#ifdef	__cplusplus
}
#endif

#endif	/* TASK_H */

