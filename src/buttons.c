#include <stdio.h>
#include "buttons.h"
#include "dsp/synth.h"
#include "gpio.h"


/**
 * Define the hardcoded button array with associated BBB pin numbers and notes for the
 * synth to play
*/
button button_arr [] = {
    { .gpio = GPIO_PIN_INVALID, .pin_number = 44, .pressed = 0, .note = 1 }, 
    { .gpio = GPIO_PIN_INVALID, .pin_number = 26, .pressed = 0, .note = 2 }, 
    { .gpio = GPIO_PIN_INVALID, .pin_number = 46, .pressed = 0, .note = 3 }, 
    { .gpio = GPIO_PIN_INVALID, .pin_number = 65, .pressed = 0, .note = 4 }, 
};


void init_button_arr() {
    int16_t arr_length = sizeof(button_arr)/sizeof(button_arr[0]);
    for (int i=0; i < arr_length; i++) {
        button_arr[i].gpio = gpio_open(button_arr[i].pin_number, true);
    }
}


void button_tick(synth *syn, bool verbose)
{
  int16_t arr_length = sizeof(button_arr)/sizeof(button_arr[0]);
  for (int i=0; i < arr_length; i++) {
    
    bool curr_btn_read= gpio_read(button_arr[i].gpio);

    if (curr_btn_read) {
        if (!button_arr[i].pressed) { 
            synth_press(syn, button_arr[i].note); 
            if(verbose) {
                printf("Button %d pressed\n", button_arr[i].pin_number);
            }
            button_arr[i].pressed = 1;
            }
    } else {
        if ((button_arr[i].pressed)) {
            synth_release(syn, button_arr[i].note); 
            if(verbose) {
                printf("Button %d released\n", button_arr[i].pin_number);
            }
            button_arr[i].pressed = 0;
        }
    }
  }
}
