#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <string.h>

#include "app.h"

#define PRU0_FIRMWARE_NAME "vocoder-adc.pru0.firmware"
#define PRU1_FIRMWARE_NAME "vocoder-i2sv1.pru1.firmware"

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
}