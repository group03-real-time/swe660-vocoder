#include "mmap.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <string.h>
#include <sys/mman.h>

#include "app.h"

#define PRU0_FIRMWARE_NAME "vocoder-adc.pru0.firmware"
#define PRU1_FIRMWARE_NAME "vocoder-i2sv1.pru1.firmware"

static int dev_mem_fd = -1;

#define MAX_MAPPINGS 16

typedef struct {
	void *ptr;
	uintptr_t key;
} memory_mapping;

static memory_mapping mappings[MAX_MAPPINGS];

static void
mmap_init() {
	for(int i = 0; i < MAX_MAPPINGS; ++i) {
		mappings[i].ptr = NULL;
		mappings[i].key = 0;
	}

	dev_mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if(dev_mem_fd < 0) {
		app_fatal_error("could not open /dev/mem fd for memory mapped IO. try with sudo?");
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

static void
test_pru_firmware_exists() {
	int fd = open(PRU0_FIRMWARE_NAME, O_RDONLY);
	if(fd < 0) {
		app_fatal_error("could not find PRU 0 firmware file '" PRU0_FIRMWARE_NAME "'. aborting.");
	}
	close(fd);

	fd = open(PRU1_FIRMWARE_NAME, O_RDONLY);
	if(fd < 0) {
		app_fatal_error("could not find PRU 1 firmware file '" PRU1_FIRMWARE_NAME "'. aborting.");
	}
	close(fd);
}

void
hardware_init() {
	mmap_init();

	test_pru_firmware_exists();

	/* First, turn off the PRUs. This may fail if they're already off. */
	sysfs_write_string("/sys/class/remoteproc/remoteproc1/state", "stop");
	sysfs_write_string("/sys/class/remoteproc/remoteproc2/state", "stop");

	/* Delete the old firmware files from /lib/firmware. Shouldn't matter too
	 * much if this succeeds. */
	unlink("/lib/firmware/" PRU0_FIRMWARE_NAME);
	unlink("/lib/firmware/" PRU1_FIRMWARE_NAME);

	/* Instead of copying or whatever, just create syslinks to the firmware files.
	 * This needs to succeed. */
	if(link(PRU0_FIRMWARE_NAME, "/lib/firmware/" PRU0_FIRMWARE_NAME) != 0) {
		app_fatal_error("could not install PRU 0 firmware.");
	}

	if(link(PRU1_FIRMWARE_NAME, "/lib/firmware/" PRU1_FIRMWARE_NAME) != 0) {
		app_fatal_error("could not install PRU 1 firmware.");
	}

	/* Now, tell the PRU's which firmware to use. This needs to succeed */
	if(sysfs_write_string("/sys/class/remoteproc/remoteproc1/firmware", PRU0_FIRMWARE_NAME) != 0) {
		app_fatal_error("could not set PRU 0 firmware.");
	}

	if(sysfs_write_string("/sys/class/remoteproc/remoteproc2/firmware", PRU1_FIRMWARE_NAME) != 0) {
		app_fatal_error("could not set PRU 1 firmware.");
	}

	/* Finally, start up the PRU's. This also needs to succeed. */
	if(sysfs_write_string("/sys/class/remoteproc/remoteproc1/state", "start") != 0) {
		app_fatal_error("could not start PRU 0.");
	}

	if(sysfs_write_string("/sys/class/remoteproc/remoteproc2/state", "start") != 0) {
		app_fatal_error("could not start PRU 1.");
	}

	/* Initialize the gpio mmap'd regions */
	gpio_init();
	/* Initialize the pru mmap'd regions */
	pru_init();
}