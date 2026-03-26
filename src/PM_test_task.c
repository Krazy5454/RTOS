#include <PM_test_task.h>
#include <task.h>
#include <PM.h>

#define BIT_DEPTH 12
#define DIVISIONS 2^BIT_DEPTH-1
#define F_MAX PM_CLK_HZ/(2^BIT_DEPTH)

void PM_handler()
{
  static i = 0;
  //do thing
  while(!PM_FIFO_full(0))
  {
    PM_set_duty(0, SIN_WAVE[i]);
    i++;
    if (i >= SIN_WAVE_SIZE)
      i = 0;
  }
}

void pm_test_task(void *pvParameters)
{
  char buffer[128];
  const TickType_t period = pdMS_TO_TICKS(100);
  
  TickType_t lastwake = xTaskGetTickCount();
  // wait until the timeout
  vTaskDelayUntil(&lastwake,period);

  // PM_acquire(0);
  // PM_set_cycle_time(0, 1000, 30000);
  // PM_set_PDM_mode(0);
  // PM_disable_FIFO(0);
  // PM_set_duty(0, 100);
  // PM_enable(0);

  // PM_acquire(1);
  // PM_set_cycle_time(1, 1000, 30000);
  // PM_set_PDM_mode(1);
  // PM_disable_FIFO(1);
  // PM_set_duty(1, 100);
  // PM_enable(1);

  // PM_acquire(2);
  // PM_set_cycle_time(2, 1000, 100);
  // PM_set_PDM_mode(2);
  // PM_disable_FIFO(2);
  // PM_set_duty(2, 50);
  // PM_enable(2);

  while(1)
  {
    PM_acquire(0);
    PM_set_cycle_time(0, DIVISIONS, F_MAX);
    PM_set_PDM_mode(0);
    PM_set_handler(0, &PM_handler);
    int ret = PM_enable_FIFO(0);
    ASSERT (ret != 0);
    PM_enable_interrupt(0);
    PM_enable(0);
  }

  vTaskDelete(NULL); //kill itslef
}


/* Structure that will hold the TCB of the task being created. */
StaticTask_t pm_test_TCB;

/* Buffer that the task being created will use as its stack. Note this
is an array of StackType_t variables. The size of StackType_t is
dependent on the RTOS port. */
StackType_t pm_test_stack[ PM_TEST_STACK_SIZE ];

