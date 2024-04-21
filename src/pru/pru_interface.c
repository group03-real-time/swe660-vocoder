

#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <string.h>

#include <sched.h>

#include "app.h"
#include "hardware.h"

#include "dsp/dsp.h"

#include <firmware/firmware.h>

#define PRU_START 0x4A300000
#define PRU_END   0x4A37FFFF
#define PRU_SIZE (PRU_END - PRU_START) + 1

#define PRU0_FIRMWARE_NAME "vocoder-adc.pru0.firmware"
#define PRU1_FIRMWARE_NAME "vocoder-i2sv1.pru1.firmware"

static struct pru0_ds *pru_adc = NULL;
static struct pru1_ds *pru_audio = NULL;

static void
pru_test_firmware_exists() {
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

static void
pru_install() {
	/* See if the firmware file even exists. If it does, that doesn't guarantee
	 * that it will still exist by the time we try to use it, but it's still better
	 * to tell the user right away if we can manage to detect this problem. */
	pru_test_firmware_exists();

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

void
pru_init() {
	/* The firmware must be installed before anything else can happen */
	pru_install();

	/* Do pin muxing */
	sysfs_write_string("/sys/devices/platform/ocp/ocp:P8_46_pinmux/state", "pruout"); /* BCK */
	sysfs_write_string("/sys/devices/platform/ocp/ocp:P8_45_pinmux/state", "pruout"); /* DIN */
	sysfs_write_string("/sys/devices/platform/ocp/ocp:P8_43_pinmux/state", "pruout"); /* LRCK */

	unsigned char *base = mmap_get_mapping(PRU_START, PRU_SIZE);
	if(!base) {
		app_fatal_error("could not get PRU mmap'd IO");
	}
	pru_adc   = (void*)(base + PRU0_GLOBAL_DS_OFFSET);
	pru_audio = (void*)(base + PRU1_GLOBAL_DS_OFFSET);

	if(pru_adc->magic != PRU0_MAGIC_NUMBER) {
		app_fatal_error("PRU 0 magic number is wrong. The firmware may not be installed correctly.");
	}

	if(pru_audio->magic != PRU1_MAGIC_NUMBER) {
		app_fatal_error("PRU 1 magic number is wrong. The firmware may not be installed correctly.");
	}
}

void
pru_shutdown() {
	/* Stop both PRUs. You may want to disable this functionality when debugging PRUs. */
	sysfs_write_string("/sys/class/remoteproc/remoteproc1/state", "stop");
	sysfs_write_string("/sys/class/remoteproc/remoteproc2/state", "stop");
}

void
pru_audio_prepare_writing() {
	/* Reset the buffer data */
	for(int i = 0; i < AUDIO_OUT_RINGBUF_SIZE; ++i) {
		pru_audio->all_data[i] = 0;
	}

	/* Make the output pointer as far as possible from the input pointer
	 * Note this is essentially write = read - 1 */
	pru_audio->out_write = (pru_audio->out_read + AUDIO_OUT_RINGBUF_SIZE - 1) % AUDIO_OUT_RINGBUF_SIZE;
}

void
pru_audio_write(int32_t sample) {
	uint32_t u;
	memcpy(&u, &sample, sizeof(u));

	/* Each sample must be REVERSED for efficient processing by the PRU */
	uint32_t rev = 0;
	for(int i = 0; i < 32; ++i) {
		rev <<= 1;
		rev |= (u >> i) & 1;
	}

	/* Compute the pointer value for the next sample */
	uint32_t next = (pru_audio->out_write + 1) % AUDIO_OUT_RINGBUF_SIZE;

	/* Yield while the buffer is full */
	while(next == pru_audio->out_read && !pru_audio->out_empty) {
		sched_yield();
	}

	/* Write the data into the ring buffer, then increment the output pointer */
	pru_audio->all_data[pru_audio->out_write] = rev;
	pru_audio->out_write = next;
}

void
pru_audio_prepare_reading() {
	/* Reset the buffer */
	for(int i = 0; i < AUDIO_IN_RINGBUF_SIZE; ++i) {
		pru_audio->all_data[AUDIO_OUT_RINGBUF_SIZE + i] = 0;
	}

	/* Make the input pointer as far from the output pointer as possible */
	pru_audio->in_read = (pru_audio->in_write + 1) % AUDIO_IN_RINGBUF_SIZE;
}

int32_t
pru_audio_read() {
	/* Compute a gain factor to map the values to a "normalized" range of -0.5 to 0.5 */
	const dsp_num gain = (dsp_one / 2) / (2048 * AUDIO_VIRTUAL_SAMPLECOUNT);
	
	/* Wait for new data in the buffer */
	while(pru_audio->in_read == pru_audio->in_write) {
		sched_yield();
	}

	/* Read the data and update the ring buffer pointer */
	int32_t result = pru_audio->all_data[AUDIO_OUT_RINGBUF_SIZE + pru_audio->in_read];
	pru_audio->in_read = (pru_audio->in_read + 1) % AUDIO_IN_RINGBUF_SIZE;

	/* Compute the centered / normalized sample value */
	result -= (2048 * AUDIO_VIRTUAL_SAMPLECOUNT);
	return result * gain;
}

int32_t
pru_adc_read_without_reset(int32_t channel) {
	/* Reading without reset just involves reading the averaged value computed
	 * by the PRU. */
	return pru_adc->samples[channel];
}

void
pru_adc_reset(int32_t channel) {
	/* This flags this channel for reset by the PRU the next time it gets a sample
	 * for this channel. */
	pru_adc->sample_reset[channel] = 1;
}