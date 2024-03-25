/**
 * A test file for running the vocoder with the synthesizer.
 */

#include "dsp/vocoder.h"
#include "dsp/synth.h"
#include "wav/wav.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

int main_ovs(int argc, char **argv) {
	if(argc < 4) {
		printf("usage: %s -ovs <modulator.wav (voice)> <output.wav>\n", argv[0]);
		return 1;
	}

	const char *mod_fp = argv[2];
	const char *out_fp = argv[3];

	wav_io mod;
	wav_io out;

	wav_read_or_die(&mod, mod_fp);

	if(mod.sample_rate != SAMPLE_RATE) {
		printf("WARNING: modulator sample rate does not match output (%d vs %d)\n", mod.sample_rate, SAMPLE_RATE);
	}

	wav_blank_or_die(&out, 
		mod.frames,
		1,
		SAMPLE_RATE);

	/* Initialize the vocoder */
	vocoder voc;
	vc_init(&voc);

	synth syn;
	synth_init(&syn);

	/* Play some notes */
	synth_press(&syn, 0);
	synth_press(&syn, 7);
	synth_press(&syn, 12);
	synth_press(&syn, 28);

	uint64_t mi = 0;

	for(uint64_t i = 0; i < out.frames; ++i) {
		dsp_num m = (mi < mod.buffer_length) ? mod.buffer[mi] : 0;

		dsp_num c = synth_process(&syn);

		out.buffer[i] = vc_process(&voc, m, c);

		/* Only use the leftmost channel */
		mi += mod.channels;
	}

	wav_write_or_warn(&out, out_fp);

	return 0;
}