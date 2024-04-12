#include <stdio.h>
#include <unistd.h> /* usleep*/

#include "gpio.h"

int main_bg(int argc, char **argv) {
	gpio_pin in_0 = gpio_open(67, true);
	gpio_pin in_1 = gpio_open(68, true);

	gpio_pin out_0 = gpio_open(44, false);
	gpio_pin out_1 = gpio_open(26, false);

	int button_states[4] = {0};

	for(;;) {
		gpio_write(out_0, false);
		gpio_write(out_1, false);

		/* First: Read buttons associated with out0 */
		gpio_write(out_0, true);
		usleep(200);
		button_states[0] = gpio_read(in_0);
		button_states[2] = gpio_read(in_1);

		/* Then, read buttons associated with out1 */
		gpio_write(out_0, false);
		gpio_write(out_1, true);
		usleep(200);

		button_states[1] = gpio_read(in_0);
		button_states[3] = gpio_read(in_1);

		printf("state: %d %d %d %d\n", button_states[0], button_states[1], button_states[2], button_states[3]);
	}
	
	return 0;
}