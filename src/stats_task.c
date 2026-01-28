#include <stats_task.h>
#include <uart.h>
#include <task.h>
#include <stdio.h>

/* Structure that will hold the TCB of the task being created. */
StaticTask_t stats_TCB;

/* Buffer that the task being created will use as its stack. Note this is an array of
StackType_t variables. The size of StackType_t is dependent on the RTOS port. */
StackType_t stats_stack[ STATS_STACK_SIZE ];

void stats_task(void* pvParameters)
{
  char buffer[32];
  uint32_t ticks;

  while(1)
  {
 
    
    vTaskDelay(pdMS_TO_TICKS(10000)); //every 10 seconds
  } 
}
