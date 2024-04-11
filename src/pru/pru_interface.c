

#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <string.h>

#include <sched.h>

#include "../app.h"
#include "../mmap.h"

#define PRU_START 0x4A300000
#define PRU_END   0x4A37FFFF
#define PRU_SIZE (PRU_END - PRU_START) + 1

#define BUF_SIZE 512
#define DESIRED_SAMPLES 16

struct ringbuf {
	uint32_t magic;
	uint32_t in_write;
	uint32_t in_read;
	uint32_t data[BUF_SIZE];
	uint32_t write;
	uint32_t read;
	uint32_t empty;

	uint32_t indata[BUF_SIZE];

};

static struct ringbuf *pru_audio_out = NULL;

void pru_init() {
	unsigned char *base = mmap_get_mapping(PRU_START, PRU_SIZE);
	if(!base) {
		app_fatal_error("could not get PRU mmap'd IO");
	}
	pru_audio_out = (void*)(base + 0x2200);

	uint32_t magic = pru_audio_out->magic;
	uint32_t correct_magic = 0xF00DF00D; /* TODO: Sync thru headers */
	if(magic != correct_magic) {
		app_fatal_error("PRU 1 magic number is wrong. The firmware may not be installed correctly.");
	}
}

void pru_audio_prepare_latency() {
	for(int i = 0; i < BUF_SIZE; ++i) {
		pru_audio_out->data[i] = 0;
	}
	pru_audio_out->write = (pru_audio_out->read + BUF_SIZE - 1) % BUF_SIZE;
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

	uint32_t next = (pru_audio_out->write + 1) % BUF_SIZE;
	while(next == pru_audio_out->read && !pru_audio_out->empty) {
		sched_yield();
		//printf("empty = %d\n", pru_audio_out->empty);
	}

	pru_audio_out->data[pru_audio_out->write] = rev;
	pru_audio_out->write = next;
}

void pru_audio_prepare_reading() {
	for(int i = 0; i < BUF_SIZE; ++i) {
		pru_audio_out->indata[i] = 0;
	}
	pru_audio_out->in_read = pru_audio_out->in_write;
}

int32_t pru_read_audio() {
	while(pru_audio_out->in_read == pru_audio_out->in_write) {
		printf("waiting for data!\n");
		sched_yield();
	}

	int32_t result = pru_audio_out->indata[pru_audio_out->in_read];
	pru_audio_out->in_read = (pru_audio_out->in_read + 1) % BUF_SIZE;

	result -= (2048 * DESIRED_SAMPLES);

	return result;
}