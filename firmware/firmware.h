#ifndef FIRMWARE_H
#define FIRMWARE_H

/* Defines the data structures used by the PRU and by the userspace app.
 * This ensures that they can communicate with each other properly. */

#include <stdint.h>

#define AUDIO_RINGBUF_SIZE 512

/* Essentially, because the audio sampling is done over several samples,
 * but the count is nondeterministic, we want to fix it to a final count.
 *
 * This essentially corresponds to the scale of the final sample. Because,
 * for exmaple, adding 16 samples of 4096 would create a final sample of
 * 16 * 4096, and the samples will be scaled to match this scale either way. */
#define AUDIO_VIRTUAL_SAMPLECOUNT 16

struct pru0_ds {
	uint32_t magic;
	uint32_t audio_sample_avg;

	uint32_t last_audio_sample_count;

	/* set to 1 to indicate the previous sample has been read. */
	uint32_t audio_sample_reset;
	uint32_t samples[8];
};

struct pru1_ds {
	uint32_t magic;
	uint32_t in_write;
	uint32_t in_read;
	uint32_t data[AUDIO_RINGBUF_SIZE];
	uint32_t write;
	uint32_t read;
	uint32_t empty;

	uint32_t indata[AUDIO_RINGBUF_SIZE];
	uint32_t magic2;
};

/* We put the data structures at 0x200 in each PRU, as the first 0x200 bytes
 * are taken up by the stack and the heap.
 *
 * These offsets correspond to 0x200 within the PRU's local memory mapping.
 * Each PRU also sees the other PRU's memory at 0x2000. */
#define PRU_LOCAL_DS_OFFSET 0x200
#define PRU_LOCAL_OTHER_DS_OFFSET 0x2200

/* The global offset of PRU0 (offset from the starting address of the PRU memory
 * map) is 0x0, while for PRU1 it's 0x2000. Again, 0x200 is the offset of the
 * data struture inside these. */
#define PRU0_GLOBAL_DS_OFFSET 0x200
#define PRU1_GLOBAL_DS_OFFSET 0x2200

#endif