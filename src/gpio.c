#include "gpio.h"

#include <stdio.h>

#include <unistd.h>
#include <fcntl.h>
#include "types.h"
#include "app.h"

#ifdef EMULATOR

gpio_pin
gpio_open(int32_t pin_number, bool is_input) {
	/* On emulator: just track the pin number */
	return (gpio_pin){ .pin_number = pin_number, .is_input = is_input };
}

void
gpio_write(gpio_pin pin, int32_t value) {
	if(pin.is_input) {
		printf("WARNING: writing %d to input pin %d\n", value, pin.pin_number);
		return;
	}
	/* On emulator: write the pin number as a debugging message */
	printf("writing %d to pin %d\n", value, pin.pin_number);
}

bool
gpio_read(gpio_pin pin) {
	if(!pin.is_input) {
		printf("WARNING: reading from output pin %d\n", pin.pin_number);
		return false;
	}

	/* TODO: Do we have user input of some sort? Maybe consider using ncurses? */
	printf("reading from pin %d\n", pin.pin_number);
	return false;
}

void
gpio_close(gpio_pin pin) {
	/* no-op */
}

void gpio_init(){}

#else /* EMULATOR */

#ifndef USE_MMAP_GPIO /* If we're using mmap, we need to avoid defining the functions in this file. */

gpio_pin
gpio_open(int32_t pin_number, bool is_input) {
	/* Use a local buffer for the path. */
	char path_buf[64] = {0};

	/* First: open the direction file. */
	snprintf(path_buf, sizeof(path_buf), "/sys/class/gpio/gpio%d/direction", pin_number);

	int_fd dir_fd = open(path_buf, O_RDWR);
	if(dir_fd < 0) {
		app_fatal_error("could not open gpio pin direction file");
	}
	/* After opening the dir file, configure the pin as an input or output. */
	if(is_input) {
		write(dir_fd, "in", 2);
	}
	else {
		write(dir_fd, "out", 3);
	}
	close(dir_fd);

	/* Then, open the value file. */
	snprintf(path_buf, sizeof(path_buf), "/sys/class/gpio/gpio%d/value", pin_number);

	/* Open the pin_fd as other WRONLY or RDONLY depending. */
	int_fd pin_fd = -1;
	
	if(is_input) {
		pin_fd = open(path_buf, O_RDONLY);
	}
	else {
		pin_fd = open(path_buf, O_WRONLY);
	}
	if(pin_fd < 0) {
		app_fatal_error("could not open gpio pin value file");
	}

	/* Return the fd. */
	return (gpio_pin){ .fd = pin_fd };
}

void
gpio_write(gpio_pin pin, int32_t value) {
	char c = value ? '1' : '0';
	write(pin.fd, &c, 1);
}

bool
gpio_read(gpio_pin pin) {
	char buf = 0;
	/* Seek to beginning of file */
	lseek(pin.fd, 0, SEEK_SET);
	if(read(pin.fd, &buf, 1) != 1) {
		app_fatal_error("gpio_read could not read from pin fd. did you open it correctly?");
	}

	return buf == '1';
}

void
gpio_close(gpio_pin pin) {
	/* Don't close invalid files */
	if(pin.fd < 0) return;

	/* Close the pin fd */
	close(pin.fd);
}

void gpio_init(){}

#endif /* USE_MMAP_GPIO */

#endif /* EMULATOR */

