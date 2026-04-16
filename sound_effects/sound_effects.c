
#include <sounds.h>
#include <sound_effects.h>
#include <stddef.h>
#include <queue.h>
#include <PM.h>
#include <stdio.h>

#define CHANNEL 0

EventGroupHandle_t effect_events;

// Each sound effect task will send audio buffers (actually just
// pointers) to the mixer using a dedicated queue
static QueueHandle_t effect_to_mixer_queues[NUM_EFFECTS];

// The mixer will send buffer pointers to the ISR.
static QueueHandle_t MixerToISRqueue;
// The Interrupt Handler will return the buffer pointers to the mixer
// after transferring the data to the PM device.
static QueueHandle_t ISRToMixerqueue;

// Each instance of the effect_task will be given a unique sound to
// play, and a unique trigger event using the following structure. The
// pointer to an effect task's structure will be passed to it using
// the "params" mechanism in xTaskCreateStatic.
typedef struct{
  effect_buffer *buffers;
  int num_buffers;
  EventBits_t event;
  QueueHandle_t sendqueue;
}effect_param_t;

// The audio data and events that trigger them are known at compile
// time, but the queue handles must be filled in at run time by the
// effect_init() function.
static effect_param_t effect_task_params[NUM_EFFECTS] = {
  {explosion1,NUM_explosion1_BUFFERS,EXPLOSION1_EVENT,NULL},
  {fastinvader1,NUM_fastinvader1_BUFFERS,FASTINVADER1_EVENT,NULL},
  {fastinvader2,NUM_fastinvader2_BUFFERS,FASTINVADER2_EVENT,NULL},
  {fastinvader3,NUM_fastinvader3_BUFFERS,FASTINVADER3_EVENT,NULL},
  {fastinvader4,NUM_fastinvader4_BUFFERS,FASTINVADER4_EVENT,NULL},
  {invaderkilled,NUM_invaderkilled_BUFFERS,INVADERKILLED_EVENT,NULL},
  {shoot,NUM_shoot_BUFFERS,SHOOT_EVENT,NULL},
  {ufo_highpitch,NUM_ufo_highpitch_BUFFERS,UFO_HIGHPITCH_EVENT,NULL},
  {ufo_lowpitch,NUM_ufo_lowpitch_BUFFERS,UFO_LOWPITCH_EVENT,NULL}
};


// The interrupt handler for the audio pulse modulator
void audio_handler()
{
  static uint8_t* buffer = NULL; // make it static so that it always exists.
  static int buffer_valid = 0;
  static int i = 0;
  BaseType_t HigherPriorityTaskWoken;
	BaseType_t ret;

  //shouldn't call xQueueReceiveFromISR before scheduler started
  //if( shecudler not running yet) wrise zeros

  if (buffer_valid == 0) //check anything in queue
  {
    if (pdPASS == xQueueReceiveFromISR ( MixerToISRqueue, 
                                          &buffer, 
                                          &HigherPriorityTaskWoken) ) //not valid buffer beucase old or not init
    {
      buffer_valid = 1;
    }
  }

  // While the PM FIFO is not full,
  while(!PM_FIFO_full(0) && buffer_valid == 1)
  {
    //   Transfer a data item from the buffer to the FIFO
    PM_set_duty_absolute(0, buffer[i]);
    i++;
    if (i >= EFFECT_BUFFER_SIZE)
    {
      //   If you send the last item in your current buffer, then send the
      //   pointer back to the mixer and get another buffer from the
      //   mixer.
      ret = xQueueSendToBackFromISR ( ISRToMixerqueue,
                                                &buffer,
                                                &HigherPriorityTaskWoken);

      ret &= xQueueReceiveFromISR( MixerToISRqueue, 
                                  &buffer, 
                                  &HigherPriorityTaskWoken);
      if( ret == pdPASS )
        buffer_valid = 1;
      else
        buffer_valid = 0;
      i = 0;
    } 
  }
  
  //just write zeros if not valid
  if (buffer_valid == 0)
  {
    while(!PM_FIFO_full(0))
    {
      PM_set_duty_absolute(0, buffer[i]); 
    }
  }


  portYIELD_FROM_ISR(HigherPriorityTaskWoken);
}

// define the final audio depth (after the mixer) and sample frequency
#define DEPTH 8
#define FREQ  8000
#define DIVISION ( PM_CLK_HZ/( (1U << DEPTH)*FREQ ) )

// Create the audio buffers for communication between the mixer and ISR
// Using 4 buffers is only a suggestion.
#define NUM_MIXER_BUFFERS 4
static uint8_t mixer_buffers[NUM_MIXER_BUFFERS][EFFECT_BUFFER_SIZE];

// The mixer task receives data from the individual effect tasks, and
// mixes the audio data before sending it to the ISR.
static void effect_mixer_task(void *params)
{
  uint8_t* buffer;
  int i;
  BaseType_t ret;
  
  //wait a little so hardware gets started up fine
  vTaskDelay(pdMS_TO_TICKS(100));

  // Initialization:
  // Put the pointers to the NUM_MIXER_BUFFERS mixer_buffers in the PM_to_mixer queue
  for( i = 0; i < NUM_MIXER_BUFFERS; i++ )
  {
    uint8_t* temp = mixer_buffers[i];
    if( pdFAIL == xQueueSendToBack( ISRToMixerqueue,
                                    &temp, //pass it pointer right to buffer
                                    portMAX_DELAY))
    {
      ASSERT(pdFAIL == pdTRUE);
    }
  }
    
  // configure and enable the pulse modulator
  PM_acquire(0);
  PM_set_cycle_time(0, DIVISION, FREQ); //BCR + 1 = 256 (or 255 + 1)
  PM_set_PDM_mode(0);
  PM_set_handler(0, &audio_handler);
  int enabled = PM_enable_FIFO(0);
  ASSERT (enabled != 0);
  PM_enable_interrupt(0);
  PM_enable(0);

  while (1)
    {
      // Part 1:
      // Pick one of the sound effects.  For each chunk of data in the sound effect:
      //   Get a mixer buffer pointer from the ISR to mixer queue
      //   Copy (making adjustments) the data from the sound effect into it.
      //   Send the mixer buffer pointer to the mixer to ISR queue
    
      for ( i = 0; i < NUM_explosion1_BUFFERS; i++)
      {
        //it will wait here until get available buffer from ISR
        ret = xQueueReceive ( ISRToMixerqueue,
                                        &buffer,
                                        portMAX_DELAY);

        //fill buffer
        for ( int j = 0; j < EFFECT_BUFFER_SIZE; j++)   
        {
          buffer[j]   = ((uint8_t)(explosion1[i].data[j])) + 128; //signed to unsigned
        }                                

        ret &= xQueueSendToBack( MixerToISRqueue, 
                                &buffer, 
                                portMAX_DELAY);
        ASSERT(ret == pdPASS); //might to actual handle this senario
      }

      vTaskDelay(pdMS_TO_TICKS(500));

      // Part 2: (comment out part 1)
      //   Get a mixer buffer pointer from the ISR to mixer queue
      //   Get incoming data pointers from all of the sound effects queues.
      //   Add all of the incoming data streams and store the results in the mixer buffer. 
      //   Send the mixer buffer pointer to the mixer to ISR queue
    }
}


// Each sound effect is managed by an instance of this task.
static void effect_task(void *params)
{
  // typecast the params pointer so we can access our effect data
  effect_param_t *my_effect = (effect_param_t*)params;

  while(1)
    {
      // Block until my event occurs.
      // loop:
      //   send pointers to my buffers to my send queue
      //   until I have sent all of my buffers.
    }
}

#define EFFECT_BUFFER_PTR_SIZE sizeof( effect_buffer* )

// declare storage for the mixer task
#define MIXER_STACK_SIZE 1024
static TaskHandle_t mixer_task_handle;
static StackType_t  mixer_stack[MIXER_STACK_SIZE];
static StaticTask_t mixer_TCB;

// define storage for the ISR to Mixer and Mixer to ISR queues
static StaticQueue_t MixerToISRqueue_QCB, ISRToMixerqueue_QCB;
static uint8_t MixerToISRqueue_storage[NUM_MIXER_BUFFERS * EFFECT_BUFFER_PTR_SIZE];
static uint8_t ISRToMixerqueue_storage[NUM_MIXER_BUFFERS * EFFECT_BUFFER_PTR_SIZE];

//effect tasks
#define EFFECT_STACK_SIZE 512 //TODO could change this
static TaskHandle_t effect_task_handle[NUM_EFFECTS];
static StackType_t  effect_stack[MIXER_STACK_SIZE][NUM_EFFECTS];
static StaticTask_t effect_TCB[NUM_EFFECTS];

// for effect to mixer queue
#define EFFECT_QUEUE_SIZE NUM_MIXER_BUFFERS
static uint8_t effect_queue_storage [EFFECT_QUEUE_SIZE * EFFECT_BUFFER_PTR_SIZE][NUM_EFFECTS];
static StaticQueue_t effect_queue_QCB [NUM_EFFECTS];

void effect_init() // main should call this function to set up the sound effects
{
  
  int i;
  
  // create all of the queues that will be used by the effects tasks
  // to send data to the mixer. Store their handles in the
  // effect_to_mixer_queues array
  // for ( i = 0; i < NUM_EFFECTS; i++)
  // {
  //   effect_to_mixer_queues[i] = xQueueCreateStatic( EFFECT_QUEUE_SIZE, 
  //                                                   EFFECT_BUFFER_PTR_SIZE,
  //                                                   effect_queue_storage[i],
  //                                                   &effect_queue_QCB[i] );
  //   ASSERT(  effect_to_mixer_queues[i] ); //make sure they got created
  //   effect_task_params[i].sendqueue = effect_to_mixer_queues[i];
  // }
  // create the two queues to communicate between the mixer and the ISR
  MixerToISRqueue = xQueueCreateStatic( NUM_MIXER_BUFFERS, 
                                        EFFECT_BUFFER_PTR_SIZE,
                                        MixerToISRqueue_storage,
                                        &MixerToISRqueue_QCB);
  ASSERT( MixerToISRqueue ); //make sure they got created

  ISRToMixerqueue = xQueueCreateStatic( NUM_MIXER_BUFFERS, 
                                        EFFECT_BUFFER_PTR_SIZE,
                                        ISRToMixerqueue_storage,
                                        &ISRToMixerqueue_QCB);
  ASSERT( MixerToISRqueue ); //make sure they got created

  // create all of the effect tasks, giving them each a unique queue handle and
  // other parameters (effect_params)
  // char name_buffer[20];
  // for ( i = 0; i < NUM_EFFECTS; i++)
  // {
  //   sprintf(name_buffer, "effect %d", i);
  //   effect_task_handle[i] = xTaskCreateStatic(effect_task, name_buffer, EFFECT_STACK_SIZE,
	// 			   (void*)&effect_task_params[i], 6, effect_stack[i], &effect_TCB[i]); //might need to change prioirty
  // }

  // create the mixer task
  //might need to change prioirty. just need to be bigger than effect priority
  mixer_task_handle = xTaskCreateStatic(effect_mixer_task, "mixer", MIXER_STACK_SIZE,
				   NULL, 7, mixer_stack, &mixer_TCB); 
}
 


