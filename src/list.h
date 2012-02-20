#ifndef TASK_H
#define	TASK_H

#ifdef	__cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <sys/types.h>
#include <stdbool.h>
    
//ITEM
typedef struct task_item {
    int task_id;
    time_t startTime;
    time_t endTime;
    time_t nextTime;
    int  frequency;
    int timeout;
    int times;
    int runTimes;
    bool mail;
    char command[1024];
    struct task_item *next;
    struct task_item *prev;
} s_task_item;

//LIST
typedef struct task_list {
    //头
    s_task_item *head;
    //尾
    s_task_item *tail;
    //任务节点数量
    int count;
} l_task_list;

void task_init(l_task_list *task_list);
void task_add(l_task_list *task_list, s_task_item *task_item);
void task_update(s_task_item *, l_task_list *task_list);
bool task_isempty(const l_task_list *task_list);
void task_free(l_task_list *task_list);
void item_free(s_task_item *task_item, l_task_list *task_list);

#ifdef	__cplusplus
}
#endif

#endif	/* TASK_H */

