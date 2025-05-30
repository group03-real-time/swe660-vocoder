/**
 * A test file for testing just the synthesizer code.
 */

#include "dsp/synth.h"
#include "dsp/dsp_perf.h"
#include "wav/wav.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

int main_os(int argc, char **argv) {
	if(argc < 3) {
		printf("usage: %s -os <output.wav>\n", argv[0]);
		return 1;
	}

	const char *out_fp = argv[2];

	wav_io out;

	wav_blank_or_die(&out, 
		SAMPLE_RATE * 60 * 5, /* 5 minutes */
		1,
		SAMPLE_RATE);

	/* Initialize the vocoder */
	synth syn;
	synth_init(&syn);

	audio_params ap;
	audio_params_default(&ap);

	ap.attack = dsp_from_double(0.00001);//dsp_from_double(0.0000001);//dsp_from_double(0.0002);
	//ap.release = dsp_from_double(0.0002);
	ap.decay = dsp_from_double(1); //dsp_from_double(1.0);
	ap.sustain = dsp_from_double(0.0);

	//int octave = 12 * 4;

	/* Play a note */
	//synth_press(&syn, octave);
	synth_press(&syn, 0);
	synth_press(&syn, 7);
	synth_press(&syn, 12);
	synth_press(&syn, 28);
	uint64_t timer = SAMPLE_RATE * 2;

	for(uint64_t i = 0; i < out.frames; ++i) {
		out.buffer[i] = synth_process(&syn, &ap);

		timer -= 1;

		/* After some seconds, play another note */
		if(timer == 0) {
			/* major fifth */
			//synth_press(&syn, octave + 7);
		}
	}

	wav_write_or_warn(&out, out_fp);

	return 0;
}