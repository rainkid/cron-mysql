#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>

#include "util.h"
#include "list.h"

/* 任务初始化 */
void init_task(l_task_list * task_list) {
	task_list->count = 0;
	task_list->head = NULL;
	task_list->tail = NULL;
}
/* 添加节点到尾部 */
void add_task(l_task_list * task_list, s_task_item * task_item) {
	task_item->next = NULL;
	task_item->prev = NULL;

	if (false == task_isempty(task_list)) {
		task_list->tail->next = task_item;
		task_item->prev = task_list->tail;
	} else {
		task_list->head = task_item;
	}

	task_list->tail = task_item;
	(task_list->count)++;
}

/* 更新节点 */
void update_task(s_task_item * task_item, l_task_list *task_list) {
	if (false == task_isempty(task_list)) {
		task_item->next = NULL;
		task_item->prev = NULL;
		s_task_item *item = task_list->head;

		if (task_item->nextTime >= task_list->head->nextTime) {
			if (task_item->nextTime < task_list->tail->nextTime) {
				while (NULL != item) {
					if (task_item->nextTime < item->nextTime) {
						task_item->prev = item->prev;
						item->prev->next = task_item;
						item->prev = task_item;
						task_item->next = item;
						break;
					}
					item = item->next;
				}
			} else {
				task_item->prev = task_list->tail;
				task_list->tail->next = task_item;
				task_list->tail = task_item;
			}
		} else {
			task_item->next = task_list->head;
			task_list->head->prev = task_item;
			task_list->head = task_item;
		}
		(task_list->count)++;
	} else {
		add_task(task_list, task_item);
	}
}

/* 任务列表是否空 */
bool task_isempty(const l_task_list *task_list) {
	return (task_list->count == 0) ? true : false;
}

/* 任务列表销毁 */
void free_task(l_task_list *task_list) {
	if (NULL != task_list && false == task_isempty(task_list)) {
		s_task_item *item;
		item = malloc(sizeof(s_task_item *));
		while (NULL != task_list->head) {
			item = task_list->head->next;
			free_item(task_list->head, task_list);
			task_list->head = item;
		}
		free_item(item, task_list);
	}
	task_list->count = 0;
	task_list->head = NULL;
	task_list->tail = NULL;
	free(task_list);
}

/* 任务节点销毁 */
void free_item(s_task_item *task_item, l_task_list *task_list) {
	(task_list->count)--;
	free(task_item->command);
	task_item->command = NULL;
	free(task_item);
}

void init_task_item(s_task_item *task_item) {
	task_item->task_id = 0;
	task_item->startTime = 0;
	task_item->endTime = 0;
	task_item->nextTime = 0;
	task_item-> frequency = 0;
	task_item->timeout = 0;
	task_item->times = 0;
	task_item->runTimes = 0;
	task_item->runTimes = false;
	task_item->command = NULL;
	task_item->next = NULL;
	task_item->prev = NULL;
}
