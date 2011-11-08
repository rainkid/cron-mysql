#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include "task.h"

/**
 * 任务初始化
 * @param tasklist
 * @return bool
 */
bool InitializeTaskList(TaskList * tasklist) {
    tasklist = NULL;
    
    return true;
}

/**
 * 添加节点到尾部
 * @param tasklist
 * @param taskitem
 * @return bool
 */
bool Add(TaskList * tasklist, TaskItem * taskitem) {
    taskitem->next = NULL;
    taskitem->prev = NULL;
    
    if (false == TaskListIsEmpty(tasklist)) {
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

/**
 * 更新节点
 * @param taskitem
 * @param tasklist
 * @return bool
 */
bool Update(TaskItem * taskitem, TaskList *tasklist) {
    if (false == TaskListIsEmpty(tasklist)) {
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
        Add(tasklist,taskitem);
    }

    return true;
}

/**
 * 任务列表是否空
 * @param tasklist
 * @return bool
 */
bool TaskListIsEmpty(const TaskList *tasklist) {
    return (tasklist->count == 0) ? true : false;
}

/**
 * 任务列表销毁
 * @param tasklist
 * @return bool
 */
bool TaskListFree(TaskList *tasklist) {
    if (false == TaskListIsEmpty(tasklist)) {
        TaskItem *temp;
        while(NULL != tasklist->head) {
            temp = tasklist->head->next;
            TaskItemFree(tasklist->head);
            tasklist->head = temp;
        }
    }
    tasklist->count = 0;
    tasklist->head = NULL;
    tasklist->tail = NULL;
    free(tasklist);
    
    return true;
}

/**
 * 任务节点销毁
 * @param tasklist
 * @return bool
 */
bool TaskItemFree(TaskItem *taskitem) {
    free(taskitem);
    
    return true;
}
