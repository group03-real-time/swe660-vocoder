#ifndef BUTTONS_H
#define BUTTONS_H

#include "gpio.h"
#include "types.h"

#include "dsp/synth.h"

#define BUTTON_DEBOUNCE 8
#define BUTTON_DEBOUNCE_MAJORITY 8

typedef struct
{
    gpio_pin gpio;
    
    bool pressed;
	bool debounce[BUTTON_DEBOUNCE];
    
	int16_t pin_number;
	int16_t note;
} button;

/**
 * Initializes the button array by opening all the required gpio pins and setting them to input mode
*/
void init_button_arr();

/**
 * Polls the specified button in the synth button array to detect if a button has been changed from released to pressed or vice versa
*/
void button_tick(synth *syn, int which, bool verbose);

#define BUTTON_COUNT 24 /* Must be up to date for polling buttons correctly */

#endif

