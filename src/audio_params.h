#ifndef AUDIO_PARAMS_H
#define AUDIO_PARAMS_H

#include "dsp/dsp.h"

#include "types.h"

#include <math.h>

typedef dsp_num (*param_adc_fn)(uint32_t value);

typedef struct {
	dsp_num semitone;
	dsp_num shape;
	dsp_num gain;
} synth_osc_params;

typedef struct {
	dsp_num attack;
	dsp_num decay;
	dsp_num sustain;
	dsp_num release;

	dsp_num output_gain;
	dsp_num noise_gain;

	synth_osc_params oscs[3];
} audio_params;

void audio_params_default(audio_params *ap);
void audio_params_init_multiplexer();
void audio_params_tick_multiplexer(audio_params *ap, bool verbose);

#endif