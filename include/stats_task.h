#ifndef STATS_TASK_H
#define STATS_TASK_H

#include <FreeRTOS.h>

//number of words in stack
#define STATS_STACK_SIZE 2048

extern StaticTask_t stats_TCB;
extern StackType_t stats_stack[ STATS_STACK_SIZE ];
void stats_task(void* pvParameters);
#endif //STATS_TASK_H