#include <stdint.h>
#include <pru_cfg.h>
#include "resource_table_empty.h"
#include "prugpio.h"

volatile register unsigned int __R30;
volatile register unsigned int __R31;

#define SQUARE_HIGH 0x4d677000
#define SQUARE_LOW  0x72988fff

#define HALF_BCK 32 /* 200 million / (44100 * 64) [actually tuned manually] */

#define CHANNEL "uninitialized"

/* Pin mapping:
 * data (DIN) = bit 0 = P8_45
 * bck (BCK) =  bit 1 = P8_46
 * lrck       = bit 2 = P8_43 */

#define BUF_SIZE 1024

struct ringbuf {
	uint32_t magic;
	uint32_t data[BUF_SIZE];
	uint32_t write;
	uint32_t read;
	uint32_t empty;
};

#define buf ((struct ringbuf *)(0x200))

void main(void) {
	uint32_t next_sample;
	uint32_t shift_sample;
	uint32_t bits;
	int i;

	buf->magic = 0xF00DF00D; /* Just so we can debug if it's working. */

	for(i = 0; i < BUF_SIZE; ++i) {
		buf->data[i] = 0;
	}
	buf->write = 0;
	buf->read = 0;
	buf->empty = 1;

	/* Initialization: set LRCK to 0 and clock out one bit (also just 0 for safety) */
	bits = 0;
	__R30 = 0;

	__delay_cycles(HALF_BCK);

	__R30 = 2;

	__delay_cycles(HALF_BCK);

	for(;;) {
		next_sample = 0;
		if(buf->read != buf->write) {
			next_sample = buf->data[buf->read];
			buf->read = (buf->read + 1) % BUF_SIZE;

			buf->empty = 0; /* Let the userspace know it can write now */
		}
		else {
			buf->empty = 1;
		}

		#undef CHANNEL
		#define CHANNEL "left"


		shift_sample = next_sample;

		/* Clock out the next sample. LRCK is already low (from last bit of prev
		 * sample). */
		for(i = 0; i < 31; ++i) {
			bits = shift_sample & 1;
			shift_sample >>= 1;

			/* LRCK = low, BCK = low, data = current */
			__R30 = (__R30 & ~0x3) | bits;

			__delay_cycles(HALF_BCK);

			/* Now, we need BCK high, everything else the same. */
			//bits |= 2;
			__R30 |= 2;

			__delay_cycles(HALF_BCK);
		}

		/* For the last bit, LRCK = high */
		bits = 0x4 | (shift_sample & 1);
		__R30 = (__R30 & ~0x3) | bits;

		__delay_cycles(HALF_BCK);
		__R30 |= 2;
		__delay_cycles(HALF_BCK);

		/* For the next 31 bits, LRCK = high */
		shift_sample = next_sample;

		#undef CHANNEL
		#define CHANNEL "right"

		for(i = 0; i < 31; ++i) {
			bits = shift_sample & 1;
			shift_sample >>= 1;

			/* LRCK = low, BCK = low, data = current */
			__R30 = (__R30 & ~0x3) | bits;

			__delay_cycles(HALF_BCK);

			/* Now, we need BCK high, everything else the same. */
			//bits |= 2;
			__R30 |= 2;

			__delay_cycles(HALF_BCK);
		}

		/* For the last bit, LRCK = low */
		bits = (shift_sample & 1);
		__R30 = (__R30 & ~0x7) | bits;

		__delay_cycles(HALF_BCK);
		__R30 |= 2;
		__delay_cycles(HALF_BCK);
		//next_sample >>= 1;
	}

	__halt();
}
// Turns off triggers
#pragma DATA_SECTION(init_pins, ".init_pins")
#pragma RETAIN(init_pins)
const char init_pins[] =  
	"/sys/class/leds/beaglebone:green:usr3/trigger\0none\0" \
	"\0\0";
