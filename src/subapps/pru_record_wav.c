#include <stdio.h>
#include <inttypes.h>

#include "wav/wav.h"
#include "pru/pru_interface.h"

int main_prw(int argc, char **argv) {
	if(argc < 3) {
		printf("usage: %s -prw <output file.wav>\n", argv[0]);
		return 1;
	}

	const char *record_fp = argv[2];

	wav_io record;

#define SAMPLE_RATE 44100
	wav_blank_or_die(&record, SAMPLE_RATE * 5, 1, SAMPLE_RATE);

	pru_audio_prepare_reading();
	for(uint64_t i = 0; i < record.frames; ++i) {
		int32_t sample = pru_audio_read();
		record.buffer[i] = sample;
	}

	printf("recording done. samples:\n");
	for(uint64_t i = 0; i < record.frames; i += 100) {
		printf("@ %" PRIu64  " -> %" PRIi32 "\n", i, record.buffer[i]);
	}

	wav_write_or_warn(&record, record_fp);

	return 0;
}