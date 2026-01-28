#ifndef HELLO_TASK_H
#define HELLO_TASK_H

#include <FreeRTOS.h>

//number of words in stack
#define HELLO_STACK_SIZE 256

extern StaticTask_t hello_TCB;
extern StackType_t hello_stack[ HELLO_STACK_SIZE ];
void hello_task(void* pvParameters);

#endif //HELLO_TASK_H