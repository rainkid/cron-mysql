/*
 * task.c
 * This file is part of <task> 
 *
 * Copyright (C) 2011 - raink.kid@gmail.com
 *
 * <task> is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * <task> is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>

#include "list.h"
/*******************************************************************/
/* 任务初始化 */
void task_init(TaskList * task_list) {
    task_list->count = 0;
	task_list->head = NULL;
	task_list->tail = NULL;
}
/*******************************************************************/
/* 添加节点到尾部 */
void task_add(TaskList * task_list, TaskItem * task_item) {
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
/*******************************************************************/
void task_copy(TaskItem *src, TaskItem *dsc){
	dsc->startTime = src->startTime;
	dsc->endTime = src->endTime;
	dsc->nextTime = src->nextTime;
	dsc->frequency = src->frequency;
	dsc->timeout = src->timeout;
	dsc->times = src->times;
	dsc->runTimes = src->runTimes;
	sprintf(dsc->command, "%s", src->command);
}
/*******************************************************************/
/* 更新节点 */
void task_update(TaskItem * task_item, TaskList *task_list) {
    if (false == task_isempty(task_list)) {
        task_item->next = NULL;
        task_item->prev = NULL;
        TaskItem *temp = task_list->head;

        if (task_item->nextTime >= task_list->head->nextTime) {
            if (task_item->nextTime < task_list->tail->nextTime) {
                while(NULL != temp) {
                    if (task_item->nextTime < temp->nextTime) {
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
        task_add(task_list,task_item);
    }
}
/*******************************************************************/
/* 任务列表是否空 */
bool task_isempty(const TaskList *task_list) {
    return (task_list->count == 0) ? true : false;
}
/*******************************************************************/
/* 任务列表销毁 */
void task_free(TaskList *task_list) {
    if (false == task_isempty(task_list)) {
        TaskItem *temp;
        while(NULL != task_list->head) {
            temp = task_list->head->next;
            item_free(task_list->head, task_list);
            task_list->head = temp;
        }
    }
    task_list->count = 0;
    task_list->head = NULL;
    task_list->tail = NULL;
    free(task_list);
}
/*******************************************************************/
/* 任务节点销毁 */
void item_free(TaskItem *task_item, TaskList *task_list) {
	(task_list->count)--;
    free(task_item);
}
