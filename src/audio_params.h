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

/**
 * Initializes the audio params with the default values.
 */
void audio_params_default(audio_params *ap);

/**
 * Initializes the multiplexer code for reading audio params.
 */
void audio_params_init_multiplexer();

/**
 * Reads a value from the multiplexer into the associated param. Note that
 * this only reads a single parameter at a time, and so should be ticked
 * at up to 16 times the desired tick rate for the desired latency.
 * 
 * For example, if you want the parameters to be sampled at 60Hz, and there
 * are 16 parameters in use, then this function must be ticked at 60Hz * 16
 * instead.
 */
void audio_params_tick_multiplexer(audio_params *ap, bool verbose);

#endif