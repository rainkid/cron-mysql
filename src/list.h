#ifndef LIST_H_
#define	LIST_H_

#include <stdio.h>
#include <sys/types.h>
#include <stdbool.h>
    
struct task_item {
    int task_id;
    time_t startTime;
    time_t endTime;
    time_t nextTime;
    int  frequency;
    int timeout;
    int times;
    int runTimes;
    bool mail;
    char *command;
    struct task_item *next;
    struct task_item *prev;
};

struct task_list {
	struct task_item *head;
	struct task_item *tail;
    int count;
};

typedef struct task_item s_task_item;
typedef struct task_list l_task_list;

void init_task(l_task_list *task_list);
void add_task(l_task_list *task_list, s_task_item *task_item);
void update_task(s_task_item *, l_task_list *task_list);
bool task_isempty(const l_task_list *task_list);
void free_task(l_task_list *task_list);
void free_item(s_task_item *task_item, l_task_list *task_list);
void init_task_item(s_task_item *task_item);

#endif	/* LIST_H_ */
