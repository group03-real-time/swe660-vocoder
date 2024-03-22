/**
 * A test file for testing that the DSP code works offline.
 * Uses the drwav.h library for reading and writing wav files.
*/

#include "dsp/vocoder.h"
#include "wav/wav.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

int main_ov(int argc, char **argv) {
	if(argc < 4) {
		printf("usage: %s -ov <modulator.wav (voice)> <carrier.wav (synth)> <output.wav>\n", argv[0]);
		return 1;
	}

	const char *mod_fp = argv[1];
	const char *car_fp = argv[2];
	const char *out_fp = argv[3];

	printf("processing with\n\tmodulator = %s\n\tcarrier = %s\n\toutput = %s\n", mod_fp, car_fp, out_fp);

	wav_io mod;
	wav_io car;
	wav_io out;

	wav_read_or_die(&mod, mod_fp);
	wav_read_or_die(&car, car_fp);

	if(mod.sample_rate != SAMPLE_RATE) {
		printf("WARNING: modulator sample rate does not match output (%d vs %d)\n", mod.sample_rate, SAMPLE_RATE);
	}
	if(car.sample_rate != SAMPLE_RATE) {
		printf("WARNING: carrier sample rate does not match output (%d vs %d)\n", car.sample_rate, SAMPLE_RATE);
	}

	wav_blank_or_die(&out, 
		(mod.frames > car.frames) ? mod.frames : car.frames,
		1,
		SAMPLE_RATE);

	/* Initialize the vocoder */
	vocoder voc;
	vc_init(&voc);

	uint64_t mi = 0;
	uint64_t ci = 0;

	for(uint64_t i = 0; i < out.frames; ++i) {
		dsp_num m = (mi < mod.frames) ? mod.buffer[mi] : 0;
		dsp_num c = (ci < car.frames) ? car.buffer[ci] : 0;

		out.buffer[i] = vc_process(&voc, m, c);

		/* Only use the leftmost channel */
		mi += mod.channels;
		ci += car.channels;
	}

	wav_write_or_warn(&out, out_fp);
}