#include "hardware.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <string.h>
#include <sys/mman.h>

#include "app.h"
#include "gpio.h"
#include "pru/pru_interface.h"

static int dev_mem_fd = -1;

#define MAX_MAPPINGS 16

typedef struct {
	void *ptr;
	uintptr_t key;
	size_t map_length;
} memory_mapping;

static memory_mapping mappings[MAX_MAPPINGS];

static void
mmap_init() {
	for(int i = 0; i < MAX_MAPPINGS; ++i) {
		mappings[i].ptr = NULL;

		/* Note: A key of 0 will not correspond to any valid key, because
		 * there's nothing interesting to map at 0 on the BeagleBone, and it's
		 * usually just NULL anyways. */
		mappings[i].key = 0;
		mappings[i].map_length = 0;
	}

	dev_mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if(dev_mem_fd < 0) {
		app_fatal_error("could not open /dev/mem fd for memory mapped IO. try with sudo?");
	}
}

static void
mmap_shutdown() {
	/* Strictly speaking, we can just let the operating system clean up the
	 * memory mappings for us. But doing it ourselves means we could also
	 * in theory support functionality for e.g. restarting the app or something. */
	for(int i = 0; i < MAX_MAPPINGS; ++i) {
		if(mappings[i].ptr) {
			munmap(mappings[i].ptr, mappings[i].map_length);
			mappings[i].ptr = NULL;
			mappings[i].key = 0;
			mappings[i].map_length = 0;
		}
	}
}

void*
mmap_get_mapping(uintptr_t key, size_t size) {
	for(int i = 0; i < MAX_MAPPINGS; ++i) {
		if(mappings[i].key == key) {
			return mappings[i].ptr;
		}
	}

	int stored_idx = 0;
	for(stored_idx = 0; stored_idx < MAX_MAPPINGS; ++stored_idx) {
		if(mappings[stored_idx].ptr == NULL) goto mmap_the_region;
	}

	app_fatal_error("too many memory mappings.");

mmap_the_region:;
	void *ptr = mmap(NULL, size, 
		PROT_READ | PROT_WRITE, /* Need to read and write. */
		MAP_SHARED,             /* We want all our changes to /dev/mem to be visible. */
		dev_mem_fd,             /* Write to /dev/mem, so that we can write to specific memory addresses. */
		(off_t)key);

	if(ptr != NULL) {
		mappings[stored_idx].ptr = ptr;
		mappings[stored_idx].key = key;
		mappings[stored_idx].map_length = size;
	}

	return ptr;
}


int
sysfs_write_string(const char *filepath, const char *string) {
	int fd = open(filepath, O_WRONLY);
	if(fd < 0) return errno;

	size_t bytes = strlen(string) + 1;
	if(write(fd, string, bytes) == -1) {
		return errno;
	}

	close(fd);
	return 0;
}

void
hardware_init() {
	mmap_init();

	/* Initialize the gpio mmap'd regions */
	gpio_init();
	/* Initialize the pru mmap'd regions */
	pru_init();
}

void
hardware_shutdown() {
	pru_shutdown();
	gpio_shutdown();

	/* Clean up all memory mappings */
	mmap_shutdown();
}