#include <FreeRTOS.h>
#include <task.h>
#include <device_addrs.h>
#include <AXI_timer.h>

/*
  In the following functions, "timer" refers to the specific timer
  that you are working with, and must be between 0 and (NUM_AXI_TIMERS-1)
  inclusive.

  There are two timer devices, and each device provides two timers.
  Each timer has three registers.  This "struct" provides bit fields
  to allow us to access the Timer Control and Status Register (TCR)
  bits of any timer individually by their name.
*/

struct TCSRbits{
  volatile unsigned MDT:1;  // Mode: Use 0 for generate mode
  volatile unsigned UDT:1;  // Up/Down Use 1 to count down
  volatile unsigned GENT:1; // Enables external generate signal
  volatile unsigned CAPT:1; // Enables external capture trigger
  volatile unsigned ARHT:1; // Auto reload. Set to 1 for recurring interrupts.
  volatile unsigned LOAD:1; // Must be 0 for timer to run, when 1, the timer 
                            // count is loaded from the LOAD register on each
                            // clock cycle
  volatile unsigned ENIT:1; // When 1, interrupts are enabled
  volatile unsigned ENT:1;  // When 1, timer is running
  volatile unsigned TINT:1; // When 1, this timer is signalling an interrupt
  volatile unsigned PWMA:1; // when 1, PWM mode is enabled (uses both timers)
  volatile unsigned ENALL:1;// When 1, all timers run (can be accessed in
                            // either TCR)
  volatile unsigned CASC:1; // This bit is only present in timer 0 TCR on each
                            // device. It is used to create a 64-bit timer,
                            // which we will not support. Always set it to zero.
};

/* The following union allows us to access the Timer Control and
   Status register (TCR) as either a 32-bit register, or access the
   bits individually by name. */
typedef union{
  volatile struct TCSRbits bits; // use .bits.* for accessing single bits
  volatile uint32_t  TCSR;       // for .TCSR for accessing all bits at once
} AXI_timer_TCSR_t;

/* This struct defines the registers in an AXI timer. */
typedef struct{
  volatile AXI_timer_TCSR_t TCSR; // Timer control and status register
  volatile uint32_t TLR;        // Timer load register
  volatile uint32_t TCR;        // Timer count register
} AXI_timer_t;

/* With the previous declarations, we can set or clear any bit in the
   TCSR using the name of the bit.  For example, the following
   statement would set the timer channel to count up:
   
   timer->TCSR.bits.UDT = 0;

   We could accomplish the same thing using a bit mask:

   timer->TCSR.TCSR &= 0xFFFFFFFD;

   But the second way is less informative to the reader.

   We can also create a local variable, set bits and assign them to
   the device all at once:

   AXI_timer_TCSR_t  tmp;  // create temporary variable
   tmp = timer->TCSR;      // read from the real TCSR
   tmp.bits.UDT = 1;       // set some bits in the temporary variable
   tmp.bits.ENIT = 1;
   tmp.bits.ENT = 1;
   timer->TCSR = tmp;      // assign to the real TCSR
   
*/   

/* 
  We have defined what a physical timer looks like.  Now we need to
  add some additional information.  Each timer device has two timers
  in it, but there is only one interrupt for the device. We
  want to track which task owns each individual timer and we want to
  allow a different interrupt handler for each individual timer.
  
  So we have to set up a handler for each timer device, which then
  calls the handler for a specific timer.  We will make it so that the
  final handlers for the individual timers can be set at run-time.

  We can define a struct for the timer device, which has two timers in
  it, and is hard-wired to one of the IRQ pins on the Cortex-M3.  */
typedef struct{
  TaskHandle_t  owner[2]; // Each individual timer (channel) may have an owner
  void (*handler[2])();   // Each individual timer (channel) can have a handler 
                          // that the owner sets at run-time.
  volatile AXI_timer_t *timer[2]; // Each individual timer (channel) has
                                   // its own unique base address.
  int NVIC_IRQ_NUM;       // The timer device is tied to one of the
                          // interrupt pins on the processor.
}AXI_timer_device_t;

/* Now we can create an array of timer devices.  Each entry in this
   array represents a device that contains two timers. If you add more
   timer devices to the design, then add more lines. and change
   NUM_AXI_TIMERS in AXI_timer.h */

static volatile AXI_timer_device_t timer_device[NUM_AXI_TIMERS/2] = {
  {NULL,NULL,NULL,NULL,TIMER0,TIMER0+0x10,TIMER0_IRQ},
  {NULL,NULL,NULL,NULL,TIMER1,TIMER1+0x10,TIMER1_IRQ}
};


/* This function handles interrupts for a timer device.  The device
   has two timers in it, so we have to check them both to see where
   the interrupt is coming from.  It is possible that they are BOTH
   signalling an interrupt. */
void AXI_timer_handler(volatile AXI_timer_device_t *device)
{
  /* Examine the device to see which timer is signalling an interrupt.
     It could be both, so lets do a loop.  We could unroll the loop
     to get more speed, but the code would probably be longer.  You
     can optimize for speed or code size.  In this case I chose to
     minimize code size. */
     for (int i=0; i<2; i++)
     {
        if (device->timer[i]->TCSR.bits.TINT == 1) //check int bit
        {
          if (device->handler[i] != NULL)
          {
            device->handler[i](); //call the handler
          }

          // Clear the interrupt in the timer device.
          device->timer[i]->TCSR.bits.TINT = 1;
        }
     }
  // clear the interrupt in the Cortex M3 NVIC. 
  NVIC_ClearPendingIRQ(device->NVIC_IRQ_NUM);
}


/* Define the ISR for timer device 0. This is the first thing that
   gets called when timer device 0 generates an interrupt. */
void AXI_TIMER_0_ISR()
{
  volatile AXI_timer_device_t *dev = &(timer_device[0]);
  // Call the timer device handler and give it the timer 0 struct
  AXI_timer_handler(dev);
}

/* Define the ISR for timer device 1 This is the first thing that
   gets called when timer device 1 generates an interrupt. */
void AXI_TIMER_1_ISR()
{
  volatile AXI_timer_device_t *dev = &(timer_device[1]);
  // Call the timer device handler and give it the timer 1 struct
  AXI_timer_handler(dev);
}

/* Allocate a timer.  Returns -1 if no timers are available. */
int AXI_TIMER_allocate()
{
  for (int dev = 0; dev < NUM_AXI_TIMERS/2; dev++) //go though each device
  {
    for (int channel = 0; channel<2; channel++) //and each channel of that device
    {
      if (timer_device[dev].owner[channel] == NULL) //if there is no owner
      {
        // we found a free timer
        // mark it as used by current task handle
        timer_device[dev].owner[channel] = xTaskGetCurrentTaskHandle(); 
        return (dev*2 + channel); //return which timer it is
      }
    }
  }

  // no timers available
  return -1;
}

/* Release a timer. */
void AXI_TIMER_free(unsigned int timer)
{
  int dev = timer / 2;
  int channel = timer % 2;
  // only release if the current task is the owner
  if (timer_device[dev].owner[channel] == xTaskGetCurrentTaskHandle())
  {
    // mark the timer as free
    timer_device[dev].owner[channel] = NULL;
    // remove any handler
    timer_device[dev].handler[channel] = NULL;
  }
}

/* Assign a functon to handle interrupts for the given timer. The
   assigned function will be called when the given timer generates an
   interrupt. */
void AXI_TIMER_set_handler(unsigned int timer, void (*handler)())
{
  int dev = timer / 2;
  int channel = timer % 2;
  
  //check if current task is the owner
  if (timer_device[dev].owner[channel] == xTaskGetCurrentTaskHandle())
  {
    timer_device[dev].handler[channel] = handler;
  }
}


// start the timer
void AXI_TIMER_enable(unsigned int timer)
{
  int dev = timer / 2;
  int channel = timer % 2;
  
  //check if current task is the owner
  if (timer_device[dev].owner[channel] == xTaskGetCurrentTaskHandle())
  {
    timer_device[dev].timer[channel]->TCSR.bits.LOAD = 0; //make sure load bit is zero so keep current count
    AXI_TIMER_enable_interrupt(timer); //enable interrupts
    timer_device[dev].timer[channel]->TCSR.bits.ENT = 1;  //enable timer
  }
}

/* stop the timer. If remove_handler != 0 then the hander is also
   unset */
void AXI_TIMER_disable(unsigned int timer, int remove_handler)
{
  int dev = timer / 2;
  int channel = timer % 2;

  if (timer_device[dev].owner[channel] == xTaskGetCurrentTaskHandle())
  {
    timer_device[dev].timer[channel]->TCSR.bits.LOAD = 0;
    timer_device[dev].timer[channel]->TCSR.bits.ENT = 0;  //disable timer

    if(remove_handler != 0)
    {
      timer_device[dev].handler[channel] = NULL;
    }
  }
}

/* enable the timer to generate interrupts */
void AXI_TIMER_enable_interrupt(unsigned int timer)
{
  int dev = timer / 2;
  int channel = timer % 2;

  if (timer_device[dev].owner[channel] == xTaskGetCurrentTaskHandle())
  {
    timer_device[dev].timer[channel]->TCSR.bits.ENIT = 1;

    // finally, enable the interrupt in the NVIC
    NVIC_EnableIRQ(timer_device[dev].NVIC_IRQ_NUM);
  }
}

/* Disable the timer interrupt   */
void AXI_TIMER_disable_interrupt(unsigned int timer)
{
  int dev = timer / 2;
  int channel = timer % 2;

  if (timer_device[dev].owner[channel] == xTaskGetCurrentTaskHandle())
  {
    timer_device[dev].timer[channel]->TCSR.bits.ENIT = 0;

    // If the other channel also has interrupts disabled, then
    // turn off interrupts in the NVIC
    if (timer_device[dev].timer[1-channel]->TCSR.bits.ENIT == 0 )
    {
      NVIC_DisableIRQ(timer_device[dev].NVIC_IRQ_NUM);
    }
  }
}

/* In the following two functions, value is set in clock cycles. Use
   AXI_TIMER_US_TO_COUNT to convert desired nanoseconds to desired timer
   count value, or use AXI_TIMER_HZ_TO_COUNT to set it in Hertz. */

/* Configure and start the timer to give repeating interrupts. */
void AXI_TIMER_set_repeating(unsigned int timer, int count)
{
  int dev = timer / 2;
  int channel = timer % 2;

  if (timer_device[dev].owner[channel] == xTaskGetCurrentTaskHandle())
  {
    timer_device[dev].timer[channel]->TCSR.bits.ARHT = 1; //loop around and reload value
    timer_device[dev].timer[channel]->TLR = count; //put in count value
    timer_device[dev].timer[channel]->TCSR.bits.UDT = 1; //count down

    AXI_TIMER_enable(timer);
    // finally, enable the interrupts in the NVIC
    NVIC_EnableIRQ(timer_device[dev].NVIC_IRQ_NUM);
  }
}

/* Configure and start the timer give a single interrupt and then stop. */
void AXI_TIMER_set_oneshot(unsigned int timer, int count)
{
  int dev = timer / 2;
  int channel = timer % 2;

  if (timer_device[dev].owner[channel] == xTaskGetCurrentTaskHandle())
  {
    timer_device[dev].timer[channel]->TCSR.bits.ARHT = 0; //hold counter
    timer_device[dev].timer[channel]->TLR = count; //put in count value
    timer_device[dev].timer[channel]->TCSR.bits.UDT = 1; //count down

    AXI_TIMER_enable(timer);
    // finally, enable the interrupts in the NVIC
    NVIC_EnableIRQ(timer_device[dev].NVIC_IRQ_NUM);
  }
}

