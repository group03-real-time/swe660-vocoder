#include <stdint.h>
#include <pru_cfg.h>
#include "resource_table_empty.h"
#include "prugpio.h"

volatile register unsigned int __R30;
volatile register unsigned int __R31;

#define SQUARE_HIGH 0x4d677000
#define SQUARE_LOW  0x72988fff

#define HALF_BCK_A 31
#define HALF_BCK_B 32 /* 200 million / (44100 * 64) [actually tuned manually] */

#define CHANNEL "uninitialized"

/* Pin mapping:
 * data (DIN) = bit 0 = P8_45
 * bck (BCK) =  bit 1 = P8_46
 * lrck       = bit 2 = P8_43 */

#define BUF_SIZE 512

struct ringbuf {
	uint32_t magic;
	uint32_t in_write;
	uint32_t in_read;
	uint32_t data[BUF_SIZE];
	uint32_t write;
	uint32_t read;
	uint32_t empty;

	uint32_t indata[BUF_SIZE];
	uint32_t magic2;
};

struct adc_sampler {
	uint32_t magic;
	uint32_t audio_sample_avg;

		uint32_t last_audio_sample_count;

	/* set to 1 to indicate the previous sample has been read. */
	uint32_t audio_sample_reset;
	uint32_t samples[8];	
};

#define buf ((struct ringbuf *)(0x200))

/* It's on the other PRU */
#define sampler ((struct adc_sampler*)(0x2200))

void main(void) {
	uint32_t next_sample;
	uint32_t shift_sample;
	uint32_t bits;
	int i;

	buf->magic = 0xF00DF00D; /* Just so we can debug if it's working. */
	buf->magic2 = 0xD00FD00F;

	for(i = 0; i < BUF_SIZE; ++i) {
		buf->data[i] = 0;
		buf->indata[i] = 0;
	}
	buf->write = 0;
	buf->read = 0;
	buf->empty = 1;

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
		__delay_cycles(HALF_BCK_B);

		/* For the next 31 bits, LRCK = high */
		shift_sample = next_sample;

		#undef CHANNEL
		#define CHANNEL "right"

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
		__delay_cycles(HALF_BCK_B);
		//next_sample >>= 1;

		/* For every sample, also read an input sample */
		uint32_t in_sample = sampler->audio_sample_avg;
		sampler->audio_sample_reset = 1;

		/* Write this to the input ring buffer */
		buf->indata[buf->in_write] = in_sample;
		buf->in_write = (buf->in_write + 1) % BUF_SIZE;
	}

	__halt();
}
// Turns off triggers
#pragma DATA_SECTION(init_pins, ".init_pins")
#pragma RETAIN(init_pins)
const char init_pins[] =  
	"/sys/class/leds/beaglebone:green:usr3/trigger\0none\0" \
	"\0\0";
