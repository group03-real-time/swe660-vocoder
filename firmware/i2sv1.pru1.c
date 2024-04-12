#include <stdint.h>
#include <pru_cfg.h>
#include "resource_table_empty.h"

#include "firmware.h"

volatile register unsigned int __R30;
volatile register unsigned int __R31;

#define HALF_BCK_A 31
#define HALF_BCK_B 32 /* 200 million / (44100 * 64) [actually tuned manually] */

/* A smaller delay for when we do something expensive before the next thing */
#define HALF_BCK_C 26

/* Pin mapping:
 * data (DIN) = bit 0 = P8_45
 * bck (BCK) =  bit 1 = P8_46
 * lrck       = bit 2 = P8_43 */

#define buf ((struct pru1_ds *)(0x200))

/* It's on the other PRU */
#define sampler ((struct pru0_ds *)(0x2200))

void main(void) {
	uint32_t next_sample;
	uint32_t shift_sample;
	uint32_t bits;
	int i;

	buf->magic = PRU1_MAGIC_NUMBER; /* Used to detect if the PRU is running properly. */

	for(i = 0; i < AUDIO_OUT_RINGBUF_SIZE; ++i) {
		buf->out_data[i] = 0;
	}
	for(i = 0; i < AUDIO_IN_RINGBUF_SIZE; ++i) {
		buf->in_data[i] = 0;
	}
	buf->out_write = 0;
	buf->out_read = 0;
	buf->out_empty = 1;

	buf->in_read = 0;
	buf->in_write = 0;

	/* Initialization: set LRCK to 0 and clock out one bit (also just 0 for safety) */
	bits = 0;
	__R30 = 0;

	__delay_cycles(HALF_BCK_A);

	__R30 = 2;

	__delay_cycles(HALF_BCK_B);

	for(;;) {
		next_sample = 0;
		if(buf->out_read != buf->out_write) {
			next_sample = buf->out_data[buf->out_read];
			//buf->out_data[buf->out_read] = 0;
			buf->out_read = (buf->out_read + 1) % AUDIO_OUT_RINGBUF_SIZE;

			buf->out_empty = 0; /* Let the userspace know it can write now */
		}
		else {
			buf->out_empty = 1;
		}

		shift_sample = next_sample;

		/* Clock out the next sample. LRCK is already low (from last bit of prev
		 * sample). */
		for(i = 0; i < 31; ++i) {
			bits = shift_sample & 1;
			shift_sample >>= 1;

			/* LRCK = low, BCK = low, data = current */
			__R30 = (__R30 & ~0x3) | bits;

			__delay_cycles(HALF_BCK_A);

			/* Now, we need BCK high, everything else the same. */
			//bits |= 2;
			__R30 |= 2;

			__delay_cycles(HALF_BCK_B);
		}

		/* For the last bit, LRCK = high */
		bits = 0x4 | (shift_sample & 1);
		__R30 = (__R30 & ~0x3) | bits;

		__delay_cycles(HALF_BCK_A);
		__R30 |= 2;
		__delay_cycles(HALF_BCK_C); /* About to do something expensive */

		/* For every sample, also read an input sample */
		uint32_t in_sample = sampler->audio_sample_avg;
		sampler->audio_sample_reset = 1;

		/* Write this to the input ring buffer */
		buf->in_data[buf->in_write] = in_sample;
		buf->in_write = (buf->in_write + 1) % AUDIO_IN_RINGBUF_SIZE;

		/* For the next 31 bits, LRCK = high */
		shift_sample = next_sample;

		for(i = 0; i < 31; ++i) {
			bits = shift_sample & 1;
			shift_sample >>= 1;

			/* LRCK = low, BCK = low, data = current */
			__R30 = (__R30 & ~0x3) | bits;

			__delay_cycles(HALF_BCK_A);

			/* Now, we need BCK high, everything else the same. */
			//bits |= 2;
			__R30 |= 2;

			__delay_cycles(HALF_BCK_B);
		}

		/* For the last bit, LRCK = low */
		bits = (shift_sample & 1);
		__R30 = (__R30 & ~0x7) | bits;

		__delay_cycles(HALF_BCK_A);
		__R30 |= 2;
		__delay_cycles(HALF_BCK_C); /* About to do something expensive when we loop around */
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
