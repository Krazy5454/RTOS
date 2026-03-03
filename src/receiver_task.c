

#include <receiver_task.h>
#include <task.h>
#include <UART_16550.h>
#include <stdio.h>

//uncommnet to use string receive
//#define STRING_RECEIVE

// "screen /dev/ttyUSB1 9600"

void receiver_task(void *pvParameters)
{
  char ch;
  char buffer[128];
  const TickType_t period = pdMS_TO_TICKS(100);
  
  while(1)
    {

      //can choose read string or get char, string will only print once enter is pressed,
      //or when the buffer if full
      //both with pause task until chars recived
      #ifdef STRING_RECEIVE
      
      UART_16550_read_string (UART0, buffer, 128, portMAX_DELAY); 

      UART_16550_write_string (UART0, buffer, portMAX_DELAY);
      UART_16550_write_string (UART0, "\r\n", portMAX_DELAY); //go to next line

      #else
      UART_16550_get_char (UART0, &ch, portMAX_DELAY); 

      //handle chars that don't work too well
      if(ch == '\177') //this is backspace
      {
        UART_16550_write_string (UART0, "\b \b", portMAX_DELAY);
      }
      else if(ch == '\r')
      {
        UART_16550_write_string (UART0, "\r\n", portMAX_DELAY);
      } 
      else
      {
        UART_16550_put_char (UART0, ch, portMAX_DELAY);
      }
      #endif
    
    }
}

/* Dimensions the buffer that the task being created will use as its
stack. NOTE: This is the number of words the stack will hold, not the
number of bytes. For example, if each stack item is 32-bits, and this
is set to 100, then 400 bytes (100 * 32-bits) will be allocated. */
#define REVEIVER_SIZE 256

/* Structure that will hold the TCB of the task being created. */
StaticTask_t receiver_TCB;

/* Buffer that the task being created will use as its stack. Note this
is an array of StackType_t variables. The size of StackType_t is
dependent on the RTOS port. */
StackType_t receiver_stack[ RECEIVER_STACK_SIZE ];

