#include "gpio.h"

enum buttonState {
    RELEASED,
    JUSTPRESSED,
    HELD,
};


typedef struct
{
    gpio_pin gpio;
    int16_t pin_number;
    enum buttonState state;
    int16_t note;

} button;


