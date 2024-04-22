#ifndef BUTTONS_H
#define BUTTONS_H

#include "gpio.h"
#include "types.h"

#include "dsp/synth.h"


typedef struct
{
    gpio_pin gpio;
    int16_t pin_number;
    bool pressed;
    int16_t note;

} button;

/**
 * Initializes the button array by opening all the required gpio pins and setting them to input mode
*/
void init_button_arr();

/**
 * Polls each button in the synth button array to detect if a button has been changed from released to pressed or vice versa
*/
void button_tick(synth *syn, bool verbose);

#endif

