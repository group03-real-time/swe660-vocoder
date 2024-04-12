#include <stdio.h>

#include "wav/wav.h"

extern void pru_audio_prepare_latency();
extern void pru_write_audio(int32_t sample);

int main_ppw(int argc, char **argv) {
	if(argc < 3) {
		printf("usage: %s -ppw <audio to play.wav>\n", argv[0]);
		return 1;
	}

	const char *play_fp = argv[2];

	wav_io play;

	wav_read_or_die(&play, play_fp);

	pru_audio_prepare_latency();
	uint64_t frame = 0;
	for(uint64_t i = 0; i < play.frames; ++i) {
		int32_t sample = play.buffer[frame];
		frame += play.channels;

		/* For now: Make samples quiet */
		//sample >>= 5;

		pru_write_audio(sample);
	}

	return 0;
}