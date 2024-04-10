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

void main(void) {
	uint32_t next_sample;
	uint32_t shift_sample;
	uint32_t bits;
	int i;

	int temporary;

	/* Initialization: set LRCK to 0 and clock out one bit (also just 0 for safety) */
	bits = 0;
	__R30 = 0;

	__delay_cycles(HALF_BCK);

	__R30 = 2;

	__delay_cycles(HALF_BCK);

	for(;;) {
		next_sample = 0;
		/*if(buf.read != buf.write) {
			next_sample = buf.buf[buf.read];
			buf.read = (buf.read + 1) % BUF_SIZE;
		}*/

		
		if(temporary >= 100) {
			temporary = 0;
		}
		if(temporary >= 50) {
			next_sample = SQUARE_HIGH;
		}
		else {
			next_sample = SQUARE_LOW;
		}
		//printf("next sample = %x, temporary = %d\n", next_sample, temporary);
		temporary += 1;

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
