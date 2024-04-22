#include <stdio.h>
#include "buttons.h"
#include "dsp/synth.h"
#include "gpio.h"

#define BUTTON(pin_number_val, note_val) { \
	.gpio = GPIO_PIN_INVALID, \
	.pin_number = pin_number_val, \
	.pressed = 0, \
	.note = note_val \
}

/**
 * Define the hardcoded button array with associated BBB pin numbers and notes for the
 * synth to play
*/
button button_arr [] = {
    BUTTON(110, 0),
	BUTTON(111, 1),
	BUTTON(117, 2),
	BUTTON(49, 3),
	BUTTON(3, 4),
	BUTTON(4, 5),
	BUTTON(48, 6),
	BUTTON(31, 7),
	BUTTON(30, 8),
	BUTTON(112, 9),
	BUTTON(14, 10),
	BUTTON(15, 11),
	BUTTON(2, 12),
	BUTTON(5, 13),
	BUTTON(51, 14),
	BUTTON(60, 15),
	BUTTON(66, 16),
	BUTTON(69, 17),
	BUTTON(45, 18),
	BUTTON(23, 19),
	BUTTON(47, 20),
	BUTTON(27, 21),
	BUTTON(22, 22),
	BUTTON(86, 23)
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
