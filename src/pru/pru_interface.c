

#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <stdio.h>
#include <string.h>

#include <sched.h>

#include "../app.h"

#define PRU_START 0x4A300000
#define PRU_END   0x4A37FFFF
#define PRU_SIZE (PRU_END - PRU_START) + 1

/* TODO: Share file descriptors with mmap, and cleanup, and so forth */
void *get_pru_mapping() {
	int fd = open("/dev/mem", O_SYNC | O_RDWR);

	if(fd < 0) {
		app_fatal_error("pru: could not open /dev/mem. try with sudo?");
	}

	void *mapping = mmap(NULL, PRU_SIZE, 
		PROT_READ | PROT_WRITE, /* Need to read and write. */
		MAP_SHARED,             /* We want all our changes to /dev/mem to be visible. */
		fd,             /* Write to /dev/mem, so that we can write to specific memory addresses. */
		PRU_START);

	if(mapping == NULL) {
		app_fatal_error("pru: could not mmap pru memory.");
	} 
	
	return mapping;
}

#define BUF_SIZE 1024

struct ringbuf {
	uint32_t magic;
	uint32_t data[BUF_SIZE];
	uint32_t write;
	uint32_t read;
	uint32_t empty;
};

static struct ringbuf *pru_audio_out = NULL;

void get_pru_audio() {
	if(pru_audio_out == NULL) {
		unsigned char *base = get_pru_mapping();

		printf("got PRU mapping @ %p\n", base);

		pru_audio_out = (void*)(base + 0x2200); /* It's on PRU1, 0x200 bytes in */

		printf("pru_audio_out = %p\n", pru_audio_out);
		printf("magic = %x\n", pru_audio_out->magic);
	}
}

void pru_audio_prepare_latency() {
	get_pru_audio();

	for(int i = 0; i < BUF_SIZE; ++i) {
		pru_audio_out->data[i] = 0;
	}
	pru_audio_out->write = (pru_audio_out->read + BUF_SIZE - 1) % BUF_SIZE;
}

void pru_write_audio(int32_t sample) {
	get_pru_audio();

	

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