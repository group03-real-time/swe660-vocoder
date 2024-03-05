#ifndef GPIO_H
#define GPIO_H

#include <stddef.h> /* NULL */
#include "types.h"

/* gpio.h: contains an abstracted interface for GPIO operations. The interface
 * may be implemented as an emulator interface that does nothing but print to
 * the screen. 
 * 
 * It may also be implemented as an actual hardware interface, either through
 * the /sys/class/gpio filesystem, or through MMAP. */

#ifdef EMULATOR

/**
 * The gpio_pin struct is the main way to interact with GPIO pins. The struct
 * allows writing to a specific pin. The pin must be "opened" with gpio_open()
 * and "closed" with gpio_close(), which are akin to open() and close() in the
 * case of a GPIO pin implemented with a file descriptor.
 * 
 * More generally, open() and close() acquire the necessary resources to use
 * the pin and correspondingly clean them up.
 */
typedef struct {
	/** On emulator, we just track the pin number so we can print it to the screen. */
	int32_t pin_number;

	/** 
	 * Also on emulator track whether the pin is opened as an input so that we 
	 * can print an error otherwise.
	 */
	bool is_input;
} gpio_pin;

/** 
 * GPIO_PIN_INVALID should be used to reset values of type gpio_pin similarly
 * to resetting a pointer to NULL or an fd to -1.
 */
#define GPIO_PIN_INVALID (gpio_pin){ .pin_number = -1 }

#else

#ifdef USE_MMAP_GPIO

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

#define GPIO_PIN_INVALID (gpio_pin){ .dataout_ptr = NULL, .mask = 0 }

#else

typedef struct {
	/** 
	 * When implemented through /sys/class/gpio, the gpio pins are simply a
	 * file descriptor, to the gpioNUM/value file. This allows us to use write().
	 */
	int_fd fd;
} gpio_pin;

#define GPIO_PIN_INVALID (gpio_pin){ .fd = -1 }

#endif

#endif

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