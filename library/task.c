/*
 * send_mail.c
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

#include "task.h"

/* 任务初始化 */
bool Task_Init(TaskList * tasklist) {
    tasklist = NULL;
    return true;
}

/* 添加节点到尾部 */
bool Task_Add(TaskList * tasklist, TaskItem * taskitem) {
    taskitem->next = NULL;
    taskitem->prev = NULL;
    
    if (false == Task_IsEmpty(tasklist)) {
        tasklist->tail->next = taskitem;
        taskitem->prev = tasklist->tail;
    }
    else {
        tasklist->head = taskitem;
    }

    tasklist->tail = taskitem;
    (tasklist->count)++;
    
    return true;
}

Task_Next(){
	
}

/* 更新节点 */
bool Task_Update(TaskItem * taskitem, TaskList *tasklist) {
    if (false == Task_IsEmpty(tasklist)) {
        taskitem->next = NULL;
        taskitem->prev = NULL;

        if (taskitem->nextTime >= tasklist->head->nextTime) {
            if (taskitem->nextTime < tasklist->tail->nextTime) {
                TaskItem *temp = tasklist->head;
                while(NULL != temp) {
                    if (taskitem->nextTime < temp->nextTime) {
                        taskitem->prev = temp->prev;
                        temp->prev->next = taskitem;
                        temp->prev = taskitem;
                        taskitem->next = temp;
                        break;
                    }
                    temp = temp->next;
                }
            }
            else {
                taskitem->prev = tasklist->tail;
                tasklist->tail->next = taskitem;
                tasklist->tail = taskitem;
            }
        }
        else {
            taskitem->next = tasklist->head;
            tasklist->head->prev = taskitem;
            tasklist->head = taskitem;
        }
        (tasklist->count)++;
    }
    else {
        Task_Add(tasklist,taskitem);
    }

    return true;
}

/* 任务列表是否空 */
bool Task_IsEmpty(const TaskList *tasklist) {
    return (tasklist->count == 0) ? true : false;
}

/* 任务列表销毁 */
bool Task_Free(TaskList *tasklist) {
    if (false == Task_IsEmpty(tasklist)) {
        TaskItem *temp;
        while(NULL != tasklist->head) {
            temp = tasklist->head->next;
            Item_Free(tasklist->head);
            tasklist->head = temp;
        }
    }
    tasklist->count = 0;
    tasklist->head = NULL;
    tasklist->tail = NULL;
    free(tasklist);
    
    return true;
}

/* 任务节点销毁 */
bool Item_Free(TaskItem *taskitem) {
    free(taskitem);
    return true;
}
