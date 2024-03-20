#if defined(USE_MMAP_GPIO) && !defined(EMULATOR) /* Only use this code if we provide a specific compiler flag */

#include "gpio.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "app.h"

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
 * Holds reference counts for each address. Used to unmap them when they're no
 * longer used.
 */
static int32_t gpio_refcounts[4] = {0};

/** 
 * This is a lookup table for the GPIO_START for each register set (0, 1, 2, 3).
 * Needed to elegantly mmap those addresses.
 */
static const off_t gpio_start_addresses[4] = { GPIO0_START, GPIO1_START, GPIO2_START, GPIO3_START };

static volatile void *adc_address = NULL;

/** 
 * We need to open() /dev/mem in order to use the memory mapped registers.
 * This fd is used throughout the program unless all refcounts drop to 0.
 */
static int_fd dev_mem_fd = -1;

void
gpio_get_devmem() {
	/* If we don't have an FD for /dev/mem, we need to obtain one. */
	if(dev_mem_fd < 0) {
		dev_mem_fd = open("/dev/mem", O_RDWR);

		if(dev_mem_fd < 0) {
			app_fatal_error("could not open /dev/mem fd for memory mapped IO");
		}
	}
}

/**
 * A helper function to obtain the GPIO_START for a given index [0-3].
 * Increments refcounts and obtains the dev_mem_fd and mmaps as necessary.
 */
static volatile void*
gpio_get_mapping(int32_t index) {
	/* First, increment refcount always. */
	gpio_refcounts[index] += 1;

	/* If we've already mmap'd the address, we're done. */
	if(gpio_addresses[index] != NULL) {
		return gpio_addresses[index];
	}

	gpio_get_devmem();

	/* Try to obtain the mapping. */
	void *mapping = mmap(NULL, GPIO_LENGTH, 
		PROT_READ | PROT_WRITE, /* Need to read and write. */
		MAP_SHARED,             /* We want all our changes to /dev/mem to be visible. */
		dev_mem_fd,             /* Write to /dev/mem, so that we can write to specific memory addresses. */
		gpio_start_addresses[index]); /* The offset inside /dev/mem is given by this table. */

	/* Fatal error if we can't map. */
	if(!mapping) {
		app_fatal_error("could not mmap for memory mapped IO");
	}

	/* Keep track of the mapped address and return the mapping. */
	gpio_addresses[index] = mapping;
	return mapping;
}

/**
 * Helper function to stop using the mapping for a given index. Decreases
 * the refcount and cleans up if necessary.
 */
static void
gpio_release_mapping(int32_t index) {
	/* Decrease refcount. */
	gpio_refcounts[index] -= 1;

	/* If there's no more references... */
	if(gpio_refcounts[index] <= 0) {

		/* Unmap the address. */
		if(gpio_addresses[index] != NULL) {
			munmap((void*)gpio_addresses[index], GPIO_LENGTH);
			gpio_addresses[index] = NULL;
		}

		/* Now check if we need to clean up the fd. */
		if(dev_mem_fd > -1) {
			/* If all the gpio addresses are NULL, we should clean it up. */
			for(int32_t i = 0; i < 4; ++i) {
				if(gpio_addresses[i] != NULL) return;
			}

			/* Close the FD -- it was > -1, but all the addresses were NULL. */
			close(dev_mem_fd);
			dev_mem_fd = -1;
		}
	}
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
	/* In order to keep the pin "lean" for actually writing to the GPIO...
	 * let's just loop through the addresses, find the one that matches, and
	 * release that one. */

	/* Note this only works because the addresses are increasing in value. */

	/* The location of the DATAIN or DATAOUT should be > than it's start address */
	uintptr_t target = (uintptr_t)(pin.data_ptr);

	for(int32_t i = 0; i < 4; ++i) {
		/* If we find a gpio_start_address that is below the dataout pointer... */
		if(target > (uintptr_t)gpio_start_addresses[i]) {
			/* Then we've found our start address, and release that mapping. */
			gpio_release_mapping(i);
			return;
		}
	}
}

#define ADC_LENGTH 4096
#define ADC_START  0x44E0D000 

#define ADC_STEPCONFIG1 0x68
#define ADC_STEPDELAY1  0x6C

void
gpio_analog_open() {
	gpio_get_devmem();

	if(!adc_address) {
		adc_address = mmap(NULL, ADC_LENGTH, 
		PROT_READ | PROT_WRITE, /* Need to read and write. */
		MAP_SHARED,             /* We want all our changes to /dev/mem to be visible. */
		dev_mem_fd,             /* Write to /dev/mem, so that we can write to specific memory addresses. */
		ADC_START); 

		if(!adc_address) {
			app_fatal_error("could not open adc");
		}
	}


}

#endif /* USE_MMAP_GPIO */