#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>

#include "define.h"
#include "list.h"

/* 任务初始化 */
void init_task(lt_task_list * task_list) {
    task_list->count = 0;
	task_list->head = NULL;
	task_list->tail = NULL;
}
/* 添加节点到尾部 */
void add_task(lt_task_list * task_list, st_task_item * task_item) {
    task_item->next = NULL;
    task_item->prev = NULL;

    if (false == task_isempty(task_list)) {
        task_list->tail->next = task_item;
        task_item->prev = task_list->tail;
    }
    else {
        task_list->head = task_item;
    }

    task_list->tail = task_item;
    (task_list->count)++;
}

/* 更新节点 */
void update_task(st_task_item * task_item, lt_task_list *task_list) {
    if (false == task_isempty(task_list)) {
        task_item->next = NULL;
        task_item->prev = NULL;
        st_task_item *temp = task_list->head;

        if (task_item->next_time >= task_list->head->next_time) {
            if (task_item->next_time < task_list->tail->next_time) {
                while(NULL != temp) {
                    if (task_item->next_time < temp->next_time) {
                        task_item->prev = temp->prev;
                        temp->prev->next = task_item;
                        temp->prev = task_item;
                        task_item->next = temp;
                        break;
                    }
                    temp = temp->next;
                }
            }
            else {
                task_item->prev = task_list->tail;
                task_list->tail->next = task_item;
                task_list->tail = task_item;
            }
        }
        else {
            task_item->next = task_list->head;
            task_list->head->prev = task_item;
            task_list->head = task_item;
        }
        (task_list->count)++;
    }
    else {
        add_task(task_list,task_item);
    }
}

/* 任务列表是否空 */
bool task_isempty(const lt_task_list *task_list) {
    return (task_list->count == 0) ? true : false;
}

/* 任务列表销毁 */
void free_task(lt_task_list *task_list) {
    if (NULL != task_list && false == task_isempty(task_list)) {
        st_task_item *temp;
        while(NULL != task_list->head) {
            temp = (st_task_item *)task_list->head->next;
            free_item(task_list->head);
            task_list->head = temp;
        }
        free_item(temp);
    }
    task_list->count = 0;
    task_list->head = NULL;
    task_list->tail = NULL;
    free_var(task_list);
}

/* 任务节点销毁 */
void free_item(st_task_item *task_item) {
	free_var(task_item->command);
	task_item->command = NULL;
	free_var(task_item);
}


void delete_item(st_task_item *task_item, lt_task_list *task_list)	{
	 if (NULL != task_list && false == task_isempty(task_list)) {
		st_task_item *temp;
		while(NULL != task_list->head) {
			temp = (st_task_item *)task_list->head->next;
			if (task_item == task_list->head) {
				free_item(task_item);
				task_list->head = temp;
				(task_list->count)--;
			}
		}
	}
};

void copy_item(st_task_item *dest, st_task_item * src){
	dest->task_id = src->task_id;
	dest->start_time = src->start_time;
	dest->end_time = src->end_time;
	dest->next_time = src->next_time;
	dest-> step = src->step;
	dest->timeout = src->timeout;
	dest->times = src->times;
	dest->run_times = src->run_times;
	dest->mail = false;
	spr_strcpy(&dest->command, src->command);
	dest->next = src->next;
	dest->prev = src->prev;
	dest->deal_func = src->deal_func;
}


void init_task_item(st_task_item *task_item) {
	task_item->task_id = 0;
	task_item->start_time = 0;
	task_item->end_time = 0;
	task_item->next_time = 0;
	task_item->step = 0;
	task_item->timeout = 0;
	task_item->times = 0;
	task_item->run_times = 0;
	task_item->mail = false;
	task_item->command = NULL;
	task_item->next = NULL;
	task_item->prev = NULL;
	task_item->deal_func = NULL;
}
