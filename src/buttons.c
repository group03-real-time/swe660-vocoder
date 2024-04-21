#include "buttons.h"
#include "dsp/synth.h"


button button_arr [] = {
    { .gpio = GPIO_PIN_INVALID, .pin_number = 44, .state = RELEASED, .note = 1 }, 
    { .gpio = GPIO_PIN_INVALID, .pin_number = 26, .state = RELEASED, .note = 2 }, 
    { .gpio = GPIO_PIN_INVALID, .pin_number = 46, .state = RELEASED, .note = 3 }, 
    { .gpio = GPIO_PIN_INVALID, .pin_number = 65, .state = RELEASED, .note = 4 }, 
};

void init_button_arr() {
    int16_t arr_length = sizeof(button_arr)/sizeof(button_arr[0]);
    for (int i=0; i < arr_length; i++) {
        button_arr[i].gpio = gpio_open(button_arr[i].pin_number, true);
    }
}


/**
 * Polls each button in the synth button array to detect if a button is being pushed, held, or released
*/
void button_tick(synth s)
{

}