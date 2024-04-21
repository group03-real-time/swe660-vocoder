#include "gpio.h"

#include "types.h"

#define COUNT 8

int pin_nuasdfsdfmbers[COUNT] = {
	75,
	77,
	79
};

int piasdfn_numbers[COUNT] = {
	75,
	77,
	79,
	80,
	81,
	11,
	89,
	88,
	61,
	//33,
	//37,
	//63
	/* BANNED! 74,*/
	76,
	78,
	8,

	9,
	10,
	87,
	86,
	
	22,
	27,
	47,
	23,
	45,

	65,//40,//36, /* ? */

	51,//51,//32, /* ? */
	60,//60,//62, /* ? */
};

int pin_numbers_set_a[COUNT] = {
	110,
	111,
	//125,//
	117,
	49,
	3,
	//13,//
	4,
	48,
	31,
	30,
	112,
	//123,
	14,
	15,
	2,
	//12,
	5,
	51,
	//40,
	60
};

int pin_numbers[COUNT] = {
	66,
	69,
	45,
	23,
	47,
	27,
	22,
	86
};

// some that don't work: 125, 13, 123, 12, 40, 62
// note: pin 5 does work, although my wire broke

gpio_pin pins[COUNT];

bool has_been_true[COUNT];

int main_bwt(int arg, char **argv) {
	for(int i = 0; i < COUNT; ++i) {
		pins[i] = gpio_open(pin_numbers[i], true);
		has_been_true[i] = false;
	}

	for(;;) {
		for(int i = 0; i < COUNT; ++i) {
			bool state = gpio_read(pins[i]);
			if(state) {
				has_been_true[i] = true;
			}
			//printf(state ? "1" : "0");
		}

		for(int i = 0; i < COUNT; ++i) {
			if(!has_been_true[i]) {
				printf("%d ", pin_numbers[i]);
			}
		}
		puts("");
	}
}