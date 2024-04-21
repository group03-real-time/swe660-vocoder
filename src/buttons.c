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
 * Polls each button in the synth button array to detect if a button has been changed from released to pressed or vice versa
*/
void button_tick(synth *syn)
{
  int16_t arr_length = sizeof(button_arr)/sizeof(button_arr[0]);
  for (int i=0; i < arr_length; i++) {
    
    bool curr_btn_read= gipo_read(button_arr[i].gpio);
    
    if (curr_btn_read) {
        if (button_arr[i].state == RELEASED) { 
            synth_press(syn, button_arr[i].note); 
            button_arr[i].state == PRESSED;
            }
    } else {
        if ((button_arr[i].state == PRESSED)) {
            synth_release(syn, button_arr[i].note); 
            button_arr[i].state == RELEASED;
        }
    }
  }
}