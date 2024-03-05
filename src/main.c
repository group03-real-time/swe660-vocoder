#include <stdio.h>
#include <assert.h>

#include "app.h"
#include "gpio.h"
#include "good_user_input.h"

#include <time.h>
#include <unistd.h>
#include <fcntl.h>

/* original run: 44100 * 5 samples in 32.46 seconds
 * so the samples_per_sec we can run with using the fd method is: 6792
 */
#define SAMPLES_PER_SEC 6792

/* 5 second audio clip */
int total_samples = SAMPLES_PER_SEC * 5;

void write_int32(FILE *file, int32_t i) {
	putc((i >> 0 ) & 0xFF, file);
	putc((i >> 8 ) & 0xFF, file);
	putc((i >> 16) & 0xFF, file);
	putc((i >> 24) & 0xFF, file);
	
	
	
}

void write_int16(FILE *file, int16_t i) {
	putc((i >> 0 ) & 0xFF, file);
	putc((i >> 8 ) & 0xFF, file);
	
}

/* https://docs.fileformat.com/audio/wav/ */
void write_wav_header(FILE *file) {
	int32_t file_size = 44 + (total_samples * 2 * 2);
	fputc('R', file);
	fputc('I', file);
	fputc('F', file);
	fputc('F', file);
	write_int32(file, file_size - 8);
	fputc('W', file);
	fputc('A', file);
	fputc('V', file);
	fputc('E', file);
	fputc('f', file);
	fputc('m', file);
	fputc('t', file);
	fputc(' ', file); /* seems like this is supposed to be a space. idk what the source website is saying lol */
	write_int32(file, 16);
	write_int16(file, 1); /* PCM */
	write_int16(file, 2); /* stereo */
	write_int32(file, SAMPLES_PER_SEC); /* sample rate */
	write_int32(file, (SAMPLES_PER_SEC * 16 * 2) / 8);
	write_int16(file, (16 * 2) / 8);
	write_int16(file, 16); /* 16 bits per sample */
	fputc('d', file);
	fputc('a', file);
	fputc('t', file);
	fputc('a', file);

	/* we'll be writing two two-byte samples for each thing */
	write_int32(file, (total_samples * 2 * 2) - 8);
}

int
main(void) {
	/* utsname */
	app_show_utsname();

	FILE *wav_file = fopen("out.wav", "wb");

	write_wav_header(wav_file);

	/* Begin the app loop. See app.h for more info */
	app_init_loop();

	
	int fd = open("/sys/bus/iio/devices/iio:device0/in_voltage0_raw", O_RDONLY);

	while(app_running) {
		//FILE *voltage = fopen("/sys/bus/iio/devices/iio:device0/in_voltage0_raw", "r");
		//fseek(voltage, 0, SEEK_SET);
		lseek(fd, 0, SEEK_SET);
		
		char buf[5] = {0};
		(void)read(fd, buf, 5);
		
		int raw_v = 2048;
		sscanf(buf, "%d", &raw_v);
		//fclose(voltage);
		raw_v -= 2048;

		int16_t sample = (int16_t)raw_v;
		write_int16(wav_file, sample);
		write_int16(wav_file, sample);
		

		//usleep(22);
		total_samples -= 1;

		//printf("total_samples = %d\n", total_samples);

		if(total_samples % 10000 == 0) {
			printf("samples = %d\n", total_samples);
		}

		if(total_samples <= 0) break;
	}
	close(fd);
	//close(fd);	
	fclose(wav_file);

	return 0;
}