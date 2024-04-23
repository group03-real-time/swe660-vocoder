#include "buttons.h"

#include <stdio.h>
#include <string.h>

#include "dsp/synth.h"
#include "gpio.h"

#define BUTTON(pin_number_val, note_val) { \
	.gpio = GPIO_PIN_INVALID, \
	.pin_number = pin_number_val, \
	.pressed = 0, \
	.note = note_val \
}

/**
 * Buttons in order of wiring:
 * 110, => 0
 * 111, => 2
 * 117, => 4
 * 49,  => 5
 * 2,   => 7
 * 5,   => 9
 * 48,  => 11
 * 31,  => 1
 * 30,  => 3
 * 112, => 6
 * 14,  => 8
 * 15,  => 10
 * 3,   => 12
 * 4,   => 14
 * 51,  => 16
 * 60,  => 17
 * 66,  => 19
 * 69,  => 21
 * 45,  => 23
 * 23,  => 13
 * 47,  => 15
 * 27,  => 18
 * 22,  => 20
 * 86   => 22
*/

/**
 * Define the hardcoded button array with associated BBB pin numbers and notes for the
 * synth to play
*/
button button_arr [] = {
    BUTTON(110, 0),
	BUTTON(111, 2),
	BUTTON(117, 4),
	BUTTON(49, 5),
	BUTTON(3, 7),
	BUTTON(5, 9),
	BUTTON(48, 11),
	BUTTON(31, 1),
	BUTTON(30, 3),
	BUTTON(112, 6),
	BUTTON(14, 8),
	BUTTON(15, 10),
	BUTTON(2, 12),
	BUTTON(4, 14),
	BUTTON(51, 16),
	BUTTON(60, 17),
	BUTTON(66, 19),
	BUTTON(69, 21),
	BUTTON(45, 23),
	BUTTON(23, 13),
	BUTTON(47, 15),
	BUTTON(27, 18),
	BUTTON(22, 20),
	BUTTON(86, 22)
};


void init_button_arr() {
    int16_t arr_length = sizeof(button_arr)/sizeof(button_arr[0]);
    for (int i=0; i < arr_length; i++) {
        button_arr[i].gpio = gpio_open(button_arr[i].pin_number, true);

		for(int j = 0; j < BUTTON_DEBOUNCE; ++j) {
			button_arr[i].debounce[j] = false;
		}
    }
}

static bool
button_update_debounce(button *b, bool *new_value) {
	memmove(b->debounce, b->debounce + 1, sizeof(*b->debounce) * (BUTTON_DEBOUNCE - 1));
	b->debounce[BUTTON_DEBOUNCE - 1] = gpio_read(b->gpio);

	int true_count = 0;

	for(int i = 0; i < BUTTON_DEBOUNCE; ++i) {
		if(b->debounce[i]) {
			true_count += 1;
		}
	}

	if(true_count >= BUTTON_DEBOUNCE_MAJORITY) {
		*new_value = true;
		return true;
	}

	if((BUTTON_DEBOUNCE - true_count) >= BUTTON_DEBOUNCE_MAJORITY) {
		*new_value = false;
		return true;
	}

	/* No valid value */
	return false;
}

void button_tick(synth *syn, bool verbose)
{
  int16_t arr_length = sizeof(button_arr)/sizeof(button_arr[0]);
  for (int i=0; i < arr_length; i++) {
    
    bool curr_btn_read = false;
	if(!button_update_debounce(&button_arr[i], &curr_btn_read)) {
		/* If there is no valid state for the button, just skip it. */
		continue;
	}

    if (curr_btn_read) {
        if (!button_arr[i].pressed) { 
            synth_press(syn, button_arr[i].note); 
            if(verbose) {
                printf("Button %d pressed\n", button_arr[i].pin_number);
            }
            button_arr[i].pressed = true;
        }
    }
	else {
        if ((button_arr[i].pressed)) {
            synth_release(syn, button_arr[i].note); 
            if(verbose) {
                printf("Button %d released\n", button_arr[i].pin_number);
            }
            button_arr[i].pressed = false;
        }
    }
  }
}
