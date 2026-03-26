#ifndef PM_H
#define PM_H

#include <FreeRTOS.h>

#define NUM_PM_CHANNELS 5

//might need to change this
#define PM_CLK_HZ 150000000

//to make mutex
void PM_init();

// Get exclusive access to the pulse modulator channel.
BaseType_t PM_acquire(int channel);

// Release the channel. This function also disables the channel.
void PM_release(int channel);

// Set the interrupt handler function for the channel.
void PM_set_handler(int channel, void (*handler)(void));

// Set the base frequency and number of divisions for the
// channel.
void PM_set_cycle_time(int channel, int divisions, int base_frequency);

// Put the channel in PDM mode.
void PM_set_PDM_mode(int channel);

// Put the channel in PWM mode.
void PM_set_PWM_mode(int channel);

// Enable the FIFO. Return zero if the handler, cycle time, or
// mode have not been set.
int PM_enable_FIFO(int channel);

// Disable the FIFO.
void PM_disable_FIFO(int channel);

// Enable output on the channel. Return zero if the channel is
// not fully configured.
int PM_enable(int channel);

// Disable output on the channel.
void PM_disable(int channel);

// Enable interrupts on the channel.
int PM_enable_interrupt(int channel);

// Disable interrupts on the channel.
void PM_disable_interrupt(int channel);

// Set the duty cycle for the channel. If the channel is in
// FIFO mode, this function writes to the FIFO. Otherwise,
// it writes directly to the Duty Cycle Register.
void PM_set_duty(int channel,int duty);

// If the channel is in FIFO mode, this function returns 1 if
// the FIFO is full. In all other cases, it returns zero.
int PM_FIFO_full(int channel);

#endif //PM_H