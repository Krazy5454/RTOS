#ifndef SOUND_EFFECTS_H
#define SOUND_EFFECTS_H

#include <FreeRTOS.h>
#include <event_groups.h>

// Set the number of individual sound effects
#define NUM_EFFECTS 9

// Define the event group and events that trigger the sound effects
#define EXPLOSION1_EVENT    0x001
#define INVADERKILLED_EVENT 0x002
#define SHOOT_EVENT         0x004 
#define UFO_HIGHPITCH_EVENT 0x008 //short high burst sound
#define UFO_LOWPITCH_EVENT  0x010 //long low sound
#define FASTINVADER1_EVENT  0x020 //faint very quick sound
#define FASTINVADER2_EVENT  0x040
#define FASTINVADER3_EVENT  0x080
#define FASTINVADER4_EVENT  0x100 //I think this is lower then the other ones

// When it is time to play a sound effect, signal the appropriate
// event on this event group.
extern EventGroupHandle_t ninvaders_to_effect_events;

// main must call this function to initialize all of the sound effects
void effect_init();

#endif
