#ifndef LIST_H_
#define	LIST_H_

#include <stdio.h>
#include <sys/types.h>
#include <stdbool.h>

typedef struct task_item st_task_item;
typedef struct task_list lt_task_list;
typedef void(*deal_func)(st_task_item *);

struct task_item {
    int task_id;
    time_t start_time;
    time_t end_time;
    time_t next_time;
    int  step;
    int timeout;
    int times;
    int run_times;
    bool mail;
    char *command;
    struct task_item *next;
    struct task_item *prev;
    deal_func deal_func;
};

struct task_list {
	struct task_item *head;
	struct task_item *tail;
    int count;
};

void init_task(lt_task_list *task_list);
void add_task(lt_task_list *task_list, st_task_item *task_item);
void update_task(st_task_item *, lt_task_list *task_list);
bool task_isempty(const lt_task_list *task_list);
void free_task(lt_task_list *task_list);
st_task_item * copy_item(st_task_item * src);
void free_item(st_task_item *task_item);
void delete_item(st_task_item *task_item, lt_task_list *task_list);
void init_task_item(st_task_item *task_item);

#endif	/* LIST_H_ */
