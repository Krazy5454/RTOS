
#include <FreeRTOS.h>
#include <task.h>
#include <UART_16550.h>
#include <PM.h>
#include <hello_task.h>
#include <stats_task.h>
//#include <receiver_task.h>
//#include <firework_task.h>
//#include <xmas_task.h>
//#include <testcurs_task.h>
//#include <nInvaders.h>
#include <PM_test_task.h>
#include <device_addrs.h>

// "screen /dev/ttyUSB1 9600"


int main( void )
{
  TaskHandle_t hello_handle = NULL;
  TaskHandle_t stats_handle = NULL;
  // TaskHandle_t receiver_handle = NULL;
  // TaskHandle_t firework_handle = NULL;
  // TaskHandle_t xmas_handle = NULL;
  // TaskHandle_t testcurs_handle = NULL;
  // TaskHandle_t ninvaders_handle = NULL;
  TaskHandle_t pm_test_handle = NULL;


  NVIC_SetPriority(UART0_IRQ,0x6); // priority for UART
  NVIC_SetPriority(UART1_IRQ,0x6); // priority for UART

  //init PM
  PM_init();
  
  // Intitialize all UARTS
  UART_16550_init(); //done by curses

  // Configure UART0 for 9600/N/8/2
  UART_16550_configure(UART0,115200,UART_PARITY_NONE,8,1);
  UART_16550_configure(UART1,115200,UART_PARITY_NONE,8,1);
  
  /* Create the task without using any dynamic memory allocation. */
  hello_handle = xTaskCreateStatic(hello_task,"hello",HELLO_STACK_SIZE,
				   NULL,3,hello_stack,&hello_TCB);
			      
  /* Create the task without using any dynamic memory allocation. */
  stats_handle = xTaskCreateStatic(stats_task,"stats",STATS_STACK_SIZE,
				   NULL,2,stats_stack,&stats_TCB); 

  /* Create the task without using anydynamic memory allocation. */
  //receiver_handle = xTaskCreateStatic(receiver_task,"receiver",RECEIVER_STACK_SIZE,
	//			   NULL,2,receiver_stack,&receiver_TCB);

  /* Create the task without using any dynamic memory allocation. */
  //firework_handle = xTaskCreateStatic(firework_task,"firework",FIREWORK_STACK_SIZE,
	//			   NULL,1,firework_stack,&firework_TCB);

  // xmas_handle = xTaskCreateStatic(xmas_task,"xmas",XMAS_STACK_SIZE,
	// 			  NULL,1,xmas_stack,&xmas_TCB);

  // testcurs_handle = xTaskCreateStatic(testcurs_task,"testcurs",TESTCURS_STACK_SIZE,
	// 			  NULL,1,testcurs_stack,&testcurs_TCB);

  // ninvaders_handle = xTaskCreateStatic(ninvaders_task,"ninvaders",NINVADERS_STACK_SIZE,
	//  			  NULL,1,ninvaders_stack,&ninvaders_TCB)
  
  pm_test_handle = xTaskCreateStatic(pm_test_task,"pm_test",PM_TEST_STACK_SIZE,
	 			  NULL,1,pm_test_stack,&pm_test_TCB);
			      
  /* start the scheduler */
  vTaskStartScheduler();

  /* we should never get to this point, but if we do, go into infinite
     loop */
  while(1);
}



/* Blatantly stolen from
   https://www.freertos.org/a00110.html#include_parameters
   and I really don't understand it yet.
*/

/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application
must provide an implementation of vApplicationGetIdleTaskMemory() to
provide the memory that is used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside
this function then they must be declared static - otherwise they will
be allocated on the stack and so not exists after this function
exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the
    Idle task's state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/*-----------------------------------------------------------*/




void vAssertCalled( unsigned line, const char * const filename )
{
  unsigned uSetToNonZeroInDebuggerToContinue=0;
    taskENTER_CRITICAL();
    {
        /* You can step out of this function to debug the assertion by using
        the debugger to set ulSetToNonZeroInDebuggerToContinue to a non-zero
        value. */
        while(uSetToNonZeroInDebuggerToContinue == 0)
        {
        }
    }
    taskEXIT_CRITICAL();
}


//stole from FREERTOS documentation
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,

                                     StackType_t **ppxTimerTaskStackBuffer,

                                     uint32_t *pulTimerTaskStackSize )

{

    /* If the buffers to be provided to the Timer task are declared inside this
       function then they must be declared static - otherwise they will be allocated on
       the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;

    static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];


    /* Pass out a pointer to the StaticTask_t structure in which the Timer
       task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;


    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;


    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
       Note that, as the array is necessarily of type StackType_t,
      configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}


void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                    char *pcTaskName )
                                    {
                                      while(1);
                                    }