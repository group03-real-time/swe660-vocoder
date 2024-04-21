#ifndef GPIO_H
#define GPIO_H

#include "types.h"

/* gpio.h: contains an abstracted interface for GPIO operations. In this project,
 * we only support the MMAP implementation for speed. */

typedef struct {
	/** 
	 * When mmap'd, we need a pointer to the GPIO memory mapped register. 
	 * Specifically, we use the DATAOUT or DATAIN register, which can directly 
	 * write or read the GPIO pins with a simple bit operation.
	 * 
	 * Note that which register is opened depends on whether the pin was opened
	 * as an input or an output. Using gpio_write() on an input or gpio_read()
	 * on an output is a bad idea.
	 */
	volatile uint32_t *data_ptr;

	/**
	 * Each DATAOUT register contains 32 pins. This mask contains the exact
	 * bitmask that we or with the register to set the pin. We can also &= ~mask
	 * to clear the pin.
	 */
	uint32_t mask;
} gpio_pin;

#define GPIO_PIN_INVALID (gpio_pin){ .data_ptr = NULL, .mask = 0 }

/**
 * Opens a gpio_pin corresponding to the given pin number. The pin number should
 * be checked with gpio_pin_valid() beforehand.
 */
gpio_pin gpio_open(int32_t pin_number, bool is_input);

/**
 * Writes a boolean value to the given gpio pin. Any nonzero value will result
 * in a 1 written to the pin, while 0 will result in a 0 written to the pin.
 */
void gpio_write(gpio_pin pin, int32_t value);

/**
 * Reads the given GPIO pin, returning true if it is high or false if it is low.
 */
bool gpio_read(gpio_pin pin);

/**
 * Closes the given gpio_pin, cleaning up any associated resources such as
 * file descriptors or mmap'd memory (when possible).
 */
void gpio_close(gpio_pin pin);

/**
 * Checks a pin number against a table of valid pin numbers, compiled in
 * gpio_pins.c. Should be used to validate pins before opening them.
 */
bool gpio_pin_valid(int32_t pin_number);

#endif