#include <stats_task.h>
#include <uart.h>
#include <task.h>
#include <stdio.h>
#include <AXI_timer.h>

/* Structure that will hold the TCB of the task being created. */
StaticTask_t stats_TCB;

/* Buffer that the task being created will use as its stack. Note this is an array of
StackType_t variables. The size of StackType_t is dependent on the RTOS port. */
StackType_t stats_stack[ STATS_STACK_SIZE ];

static int stats_count = 0;

void stats_task(void* pvParameters)
{
  char buffer[1024];
  uint32_t ticks;

  while(1)
  {
    vTaskGetRunTimeStats(buffer);
    uart_write_string("\033[2J\033[H"); //clear screen and reset cursor
    uart_write_string(
        "Task            Abs Timer       % Timer\n\r"
        "*********************************************\n\r");
    uart_write_string(buffer);
    uart_write_string("\n\r");
    vTaskDelay(pdMS_TO_TICKS(1000)); 
  } 
}

void stats_handler()
{
    stats_count++;
}


/*utils for the vTaskGetRunTimeStats*/
int get_stats_counter()
{
    return stats_count;
}

void setup_stats_timer()
{
    static int timer = -1;

    timer = AXI_TIMER_allocate(); //allocate timer 
    if ( timer != -1 ) //didn't fail
    {
        //set the timer on to loop
        AXI_TIMER_set_handler(timer, stats_handler);
        AXI_TIMER_set_repeating(timer, AXI_TIMER_HZ_TO_COUNT(configTICK_RATE_HZ*5));
    }
}
