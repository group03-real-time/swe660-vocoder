#include <stdio.h>

#include "gpio.h"
#include "types.h"

#define COUNT 24

int pin_numbers[COUNT] = {
	110,
	111,
	117,
	49,
	3,
	4,
	48,
	31,
	30,
	112,
	14,
	15,
	2,
	5,
	51,
	60,
	66,
	69,
	45,
	23,
	47,
	27,
	22,
	86
};


gpio_pin pins[COUNT];

bool has_been_true[COUNT];

int main_bwt(int arg, char **argv) {
	for(int i = 0; i < COUNT; ++i) {
		pins[i] = gpio_open(pin_numbers[i], true);
		has_been_true[i] = false;
	}

	/* The idea: Keep track of the set of pins that we have seen working, and
	 * print out all the ones we haven't seen. That way, identifying pins
	 * that don't work is as easy as pushing all the buttons and seeing
	 * which pin numbers remain. */
	for(;;) {
		for(int i = 0; i < COUNT; ++i) {
			bool state = gpio_read(pins[i]);
			if(state) {
				has_been_true[i] = true;
			}
		}

		for(int i = 0; i < COUNT; ++i) {
			if(!has_been_true[i]) {
				printf("%d ", pin_numbers[i]);
			}
		}
		puts("");
	}
}