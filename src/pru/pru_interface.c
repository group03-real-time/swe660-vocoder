

#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <string.h>

#include <sched.h>

#include "../app.h"
#include "../mmap.h"

#include <firmware/firmware.h>

#define PRU_START 0x4A300000
#define PRU_END   0x4A37FFFF
#define PRU_SIZE (PRU_END - PRU_START) + 1

static struct pru1_ds *pru_audio = NULL;

void pru_init() {
	unsigned char *base = mmap_get_mapping(PRU_START, PRU_SIZE);
	if(!base) {
		app_fatal_error("could not get PRU mmap'd IO");
	}
	pru_audio = (void*)(base + 0x2200);

	uint32_t magic = pru_audio->magic;
	uint32_t correct_magic = 0xF00DF00D; /* TODO: Sync thru headers */
	if(magic != correct_magic) {
		app_fatal_error("PRU 1 magic number is wrong. The firmware may not be installed correctly.");
	}
}

void pru_audio_prepare_latency() {
	for(int i = 0; i < AUDIO_OUT_RINGBUF_SIZE; ++i) {
		pru_audio->out_data[i] = 0;
	}
	pru_audio->out_write = (pru_audio->out_read + AUDIO_OUT_RINGBUF_SIZE - 1) % AUDIO_OUT_RINGBUF_SIZE;
}

void pru_write_audio(int32_t sample) {

	uint32_t u;
	memcpy(&u, &sample, sizeof(u));

	uint32_t rev = 0;
	for(int i = 0; i < 32; ++i) {
		//int j = (31 - i);
		rev <<= 1;
		rev |= (u >> i) & 1;
	}

	uint32_t next = (pru_audio->out_write + 1) % AUDIO_OUT_RINGBUF_SIZE;
	while(next == pru_audio->out_read && !pru_audio->out_empty) {
		sched_yield();
		//printf("empty = %d\n", pru_audio->empty);
	}

	pru_audio->out_data[pru_audio->out_write] = rev;
	pru_audio->out_write = next;
}

void pru_audio_prepare_reading() {
	for(int i = 0; i < AUDIO_IN_RINGBUF_SIZE; ++i) {
		pru_audio->in_data[i] = 0;
	}
	pru_audio->in_read = pru_audio->in_write;
}

int32_t pru_read_audio() {
	while(pru_audio->in_read == pru_audio->in_write) {
		printf("waiting for data!\n");
		sched_yield();
	}

	int32_t result = pru_audio->in_data[pru_audio->in_read];
	pru_audio->in_read = (pru_audio->in_read + 1) % AUDIO_IN_RINGBUF_SIZE;

	result -= (2048 * AUDIO_VIRTUAL_SAMPLECOUNT);

	return result;
}