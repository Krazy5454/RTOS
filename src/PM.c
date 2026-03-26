#include <FreeRTOS.h>
#include <task.h>
#include <device_addrs.h>
#include <semphr.h>
#include <PM.h>

//only can acess aquire and release if have mutex
static SemaphoreHandle_t mutex;

//control and status register
typedef struct CSR
{
    volatile unsigned OE:1; //output enable
    volatile unsigned IO:1; //invert output
    volatile unsigned SLFM:1; //synchronous load/FIFO mode
    volatile unsigned PDMM:1; //PDM mode
    volatile unsigned IE:1; //Interrupt enable
    volatile unsigned RF:1; //Reset FIFO
    volatile unsigned UNUSED_1:2;
    volatile unsigned FF:1; //FIFO full
    volatile unsigned FE:1; //FIFO empty
    volatile unsigned IA:1; //Interupt active
    volatile unsigned UNUSED_2:16;
    volatile unsigned fil:5; //FIFO interupt level
} CSR;

typedef struct PM_t
{
    volatile CSR CSR;
    volatile uint32_t CDR; //Clock divisor register
    volatile uint32_t BCR; //base cylce time register
    volatile uint32_t DCR; //duty cycle register
} PM_t;

typedef struct  init_t
{
    unsigned mode_set:1;
    unsigned BCR_set:1;
} init_t;

typedef struct PM_device_t
{
    TaskHandle_t owner;
    void (*interrupt_handler)(void);
    volatile PM_t* device;
    init_t init;
} PM_device_t;

static volatile PM_device_t PM_device[NUM_PM_CHANNELS] = {
    {NULL, NULL, PMaudio_ctrl, {}},
    {NULL, NULL, PM1_ctrl,     {}},
    {NULL, NULL, PM2_ctrl,     {}},
    {NULL, NULL, PM3_ctrl,     {}},
    {NULL, NULL, PM4_ctrl,     {}},
};

void* PM_ISR()
{
    //check all channels for intrupt bit
    for ( int channel = 0; channel < NUM_PM_CHANNELS; channel++)
    {
        if (PM_device[channel].device->CSR.IA == 1)
        {
            //Signalling intrupt so call its handler
            if (PM_device[channel].interrupt_handler != NULL)
                PM_device[channel].interrupt_handler();

            // Clear the interrupt in the timer device if handler didn't handle properly
            if (PM_device[channel].device->CSR.IA == 1)
            {
                PM_device[channel].device->CSR.IE = 0;
            }
        }
    }

    // clear the interrupt in the Cortex M3 NVIC.
    NVIC_ClearPendingIRQ(PM_IRQ);
 }

//to make mutex
void PM_init()
{
    static StaticSemaphore_t mutex_buffer;
    mutex = xSemaphoreCreateRecursiveMutexStatic(&mutex_buffer);
}

// Get exclusive access to the pulse modulator channel.
BaseType_t PM_acquire(int channel)
{
    //check channel valid and owner correct
    ASSERT(channel  >= 0 && channel < NUM_PM_CHANNELS);

    xSemaphoreTakeRecursive(mutex, portMAX_DELAY);
    //can only use owner if have mutex

    if (PM_device[channel].owner == NULL)
    {
        PM_device[channel].owner = xTaskGetCurrentTaskHandle();
    }

    xSemaphoreGiveRecursive(mutex);
}

// Release the channel. This function also disables the channel.
void PM_release(int channel)
{
    //check channel valid and owner correct
    //check channel valid and owner correct
    ASSERT(channel  >= 0 && channel < NUM_PM_CHANNELS);
    ASSERT(PM_device[channel].owner == xTaskGetCurrentTaskHandle());

    xSemaphoreTakeRecursive(mutex, portMAX_DELAY);
    //can only use owner if have mutex

    PM_device[channel].owner = NULL;
    PM_device[channel].device->CSR.OE = 0;

    xSemaphoreGiveRecursive(mutex);
}

// Set the interrupt handler function for the channel.
void PM_set_handler(int channel, void (*handler)(void))
{
    //check channel valid and owner correct
    ASSERT(channel  >= 0 && channel < NUM_PM_CHANNELS);
    ASSERT(PM_device[channel].owner == xTaskGetCurrentTaskHandle());

    PM_device[channel].interrupt_handler = handler;
}

// Set the base frequency and number of divisions for the
// channel.
void PM_set_cycle_time(int channel, int divisions, int base_frequency)
{
    //check channel valid and owner correct
    ASSERT(channel  >= 0 && channel < NUM_PM_CHANNELS);
    ASSERT(PM_device[channel].owner == xTaskGetCurrentTaskHandle());
    //check valid divisions and base_frequency
    ASSERT(divisions >= 1 && divisions <= 0x10000); //only 16 bits on divisor

    /*I commented this out and jsut assume in OE = 0*/
    // //disable output to write
    // int old_OE = PM_device[channel].device->CSR.OE;
    // PM_device[channel].device->CSR.OE = 0;

    //Clock divisor
    PM_device[channel].device->CDR = divisions - 1;

    //base cylce time 
    int base_clk =  PM_CLK_HZ/(PM_device[channel].device->CDR + 1);
    ASSERT(base_clk != 0);
    PM_device[channel].device->BCR = base_clk/base_frequency - 1; //ths equation is probably right

    PM_device[channel].init.BCR_set = 1;
    //set enable back to before
    // PM_device[channel].device->CSR.OE = old_OE;
}

// Put the channel in PDM mode.
void PM_set_PDM_mode(int channel)
{
    //check channel valid and owner correct
    ASSERT(channel  >= 0 && channel < NUM_PM_CHANNELS);
    ASSERT(PM_device[channel].owner == xTaskGetCurrentTaskHandle());

    PM_device[channel].device->CSR.PDMM = 1;
    PM_device[channel].init.mode_set = 1;
}

// Put the channel in PWM mode.
void PM_set_PWM_mode(int channel)
{
    //check channel valid and owner correct
    ASSERT(channel  >= 0 && channel < NUM_PM_CHANNELS);
    ASSERT(PM_device[channel].owner == xTaskGetCurrentTaskHandle());

    PM_device[channel].device->CSR.PDMM = 0;
    PM_device[channel].init.mode_set = 1;
}

// Enable the FIFO. Return zero if the handler, cycle time, or
// mode have not been set.
int PM_enable_FIFO(int channel)
{
    //check channel valid and owner correct
    ASSERT(channel  >= 0 && channel < NUM_PM_CHANNELS);
    ASSERT(PM_device[channel].owner == xTaskGetCurrentTaskHandle());

    if ( PM_device[channel].interrupt_handler == NULL ||
         PM_device[channel].init.BCR_set != 1 ||
         PM_device[channel].init.mode_set != 1)
        return 0;
    else      
    {
        PM_device[channel].device->CSR.SLFM = 1;
    }
    
    return 1;
}

// Disable the FIFO.
void PM_disable_FIFO(int channel)
{
    //check channel valid and owner correct
    ASSERT(channel  >= 0 && channel < NUM_PM_CHANNELS);
    ASSERT(PM_device[channel].owner == xTaskGetCurrentTaskHandle());

    PM_device[channel].device->CSR.SLFM = 0;
}

// Enable output on the channel. Return zero if the channel is
// not fully configured.
int PM_enable(int channel)
{
    //check channel valid and owner correct
    ASSERT(channel  >= 0 && channel < NUM_PM_CHANNELS);
    ASSERT(PM_device[channel].owner == xTaskGetCurrentTaskHandle());
    
    //make sure if fifo, has handler
    if ( PM_device[channel].init.BCR_set != 1 ||
         PM_device[channel].init.mode_set != 1)
        return 0;
    else
    {
        PM_device[channel].device->CSR.OE = 1;
    }      
}

// Disable output on the channel.
void PM_disable(int channel)
{
    //check channel valid and owner correct
    ASSERT(channel  >= 0 && channel < NUM_PM_CHANNELS);
    ASSERT(PM_device[channel].owner == xTaskGetCurrentTaskHandle());

    PM_device[channel].device->CSR.OE = 0;
}

// Enable interrupts on the channel.
int PM_enable_interrupt(int channel)
{
    //check channel valid and owner correct
    ASSERT(channel  >= 0 && channel < NUM_PM_CHANNELS);
    ASSERT(PM_device[channel].owner == xTaskGetCurrentTaskHandle());

    //only enable intrupts if has handler
    if (PM_device[channel].interrupt_handler != NULL)
    {
        PM_device[channel].device->CSR.IE = 1;
        NVIC_EnableIRQ(PM_IRQ); //might want to move to enable channel
    }
}

// Disable interrupts on the channel.
void PM_disable_interrupt(int channel)
{
    //check channel valid and owner correct
    ASSERT(channel  >= 0 && channel < NUM_PM_CHANNELS);
    ASSERT(PM_device[channel].owner == xTaskGetCurrentTaskHandle());

    PM_device[channel].device->CSR.IE = 0;
    NVIC_DisableIRQ(PM_IRQ);
}

// Set the duty cycle for the channel. If the channel is in
// FIFO mode, this function writes to the FIFO. Otherwise,
// it writes directly to the Duty Cycle Register.
//duty is a int 0 to 100 representing 0 to 100%
void PM_set_duty(int channel, int duty)
{
    //check channel valid and owner correct
    ASSERT(channel  >= 0 && channel < NUM_PM_CHANNELS);
    ASSERT(PM_device[channel].owner == xTaskGetCurrentTaskHandle());
    //check duty in range of BCR
   
    //int bcr = PM_device[channel].device->BCR; //debug
    duty = (PM_device[channel].device->BCR + 1) * duty / 100; //this is probably right caclulation
    ASSERT(duty >= 0 && duty <= PM_device[channel].device->BCR + 1);

    if ( duty <= PM_device[channel].device->BCR + 1 )
        PM_device[channel].device->DCR = duty;
}

// If the channel is in FIFO mode, this function returns 1 if
// the FIFO is full. In all other cases, it returns zero.
int PM_FIFO_full(int channel)
{
    //check channel valid and owner correct
    ASSERT(channel  >= 0 && channel < NUM_PM_CHANNELS);
    ASSERT(PM_device[channel].owner == xTaskGetCurrentTaskHandle());

    if ( PM_device[channel].device->CSR.SLFM == 1 &&
         PM_device[channel].device->CSR.FF == 1)
        return 1;

    return 0;
}
