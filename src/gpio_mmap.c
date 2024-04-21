#include "gpio.h"

#include <unistd.h>
#include <fcntl.h>

#include "app.h"
#include "hardware.h"

/* Memory addresses for all the GPIO registers. These are copied from the
 * AM335X Technical Reference Manual, pp. 173-185.
 *
 * They define the address of the start of each page of GPIO registers. */
#define GPIO0_START 0x44E07000U
#define GPIO0_END   0x44E07FFFU
#define GPIO1_START 0x4804C000U
#define GPIO1_END   0x4804CFFFU
#define GPIO2_START 0x481AC000U
#define GPIO2_END   0x481ACFFFU
#define GPIO3_START 0x481AE000U
#define GPIO3_END   0x481AEFFFU

/** 
 * This is the offset for the OE register inside the GPIO page. 
 * The OE register is basically used to tell whether each pin is an input or output.
 * 
 * See page 4490 and 5006 of the Technical Reference Manual.
 */
#define GPIO_OE      0x134
/**
 * This is the offset for the DATAOUT register inside the GPIO page.
 * The DATAOUT register controls the data written on each output pin. Setting
 * it to a 1 brings a pin high, and 0 brings a pin low.
 * 
 * See page 4490 and 5008 of the Technical Reference Manual.
 */
#define GPIO_DATAOUT 0x13C

/**
 * Offset for the DATAIN register. Lets us read data for input pins. Note
 * that this register is read only. I'm not sure if that means that writing to
 * it will brick the BeagleBone, but I suggest that we don't try to gpio_write()
 * to our input pins.
 * 
 * See page 4490 and 5007 of the Technical Reference Manual.
 */
#define GPIO_DATAIN 0x138

/**
 * The length of the GPIO register page. This should be equal to 4KB. It's
 * basically the exclusive range from the _START to the _END, so we subtract and
 * add 1.
 */
#define GPIO_LENGTH ((GPIO0_END - GPIO0_START) + 1)

/** Holds the mmap'd addresses for the GPIO pins. */
static volatile void *gpio_addresses[4] = { NULL, NULL, NULL, NULL };

/** 
 * This is a lookup table for the GPIO_START for each register set (0, 1, 2, 3).
 * Needed to elegantly mmap those addresses.
 */
static const uintptr_t gpio_start_addresses[4] = { GPIO0_START, GPIO1_START, GPIO2_START, GPIO3_START };

void
gpio_init() {
	for(int i = 0; i < 4; ++i) {
		gpio_addresses[i] = mmap_get_mapping(gpio_start_addresses[i], GPIO_LENGTH);
	}
}

/**
 * A helper function to obtain the GPIO_START for a given index [0-3].
 * Increments refcounts and obtains the dev_mem_fd and mmaps as necessary.
 */
static volatile void*
gpio_get_mapping(int32_t index) {
	return gpio_addresses[index];
}

gpio_pin
gpio_open(int32_t pin_number, bool is_input) {
	/* There are 32 pins per GPIO "port", so we just divide by 32. */
	volatile char *addr = gpio_get_mapping(pin_number / 32);

	/* Inside the port, we count from 0 to 31 so use modulo. This should be
	 * optimized into an & by the compiler. */
	int32_t bit = (pin_number % 32);

	/* The bitmask we will use for OE and DATAOUT is just based on the bit index. */
	uint32_t mask = (1 << bit);

	/* Find the OE and DATAOUT registers associated with this pin. */
	volatile uint32_t *gpio_oe      = (void*)(addr + GPIO_OE);
	volatile uint32_t *gpio_dataout = (void*)(addr + GPIO_DATAOUT);
	volatile uint32_t *gpio_datain  = (void*)(addr + GPIO_DATAIN);

	volatile uint32_t *result_ptr = NULL;
	
	if(is_input) {
		/* For input: we use DATAIN register
		 * Set the bit in the OE register to set the pin to an input */
		result_ptr = gpio_datain;
		*gpio_oe |= mask;
	}
	else {
		/* For output: we use DATAOUT register
		 * Clear the bit in the OE register to set the pin to an output */
		result_ptr = gpio_dataout;
		*gpio_oe &= ~mask;
	}	

	/* Return a gpio_pin with the DATAOUT and mask. */
	return (gpio_pin){ .data_ptr = result_ptr, .mask = mask };
}

void
gpio_write(gpio_pin pin, int32_t value) {
	if(value) {
		/* If we write the value, put a 1 in the DATAOUT */
		*pin.data_ptr |= pin.mask;
	}
	else {
		/* Otherwise, clear the DATAOUT */
		*pin.data_ptr &= ~pin.mask;
	}
}

bool
gpio_read(gpio_pin pin) {
	uint32_t bit = (*pin.data_ptr & pin.mask);
	return !!bit;
}

void
gpio_close(gpio_pin pin) {
	/* This is a no-op for MMAP -- instead, we just have to release the mappings
	 * when we're done with all the pins. */
}