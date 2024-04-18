#ifndef AUDIO_PARAMS_H
#define AUDIO_PARAMS_H

#include "dsp/dsp.h"

#include "types.h"

#include <math.h>

#define INPUT_MAX 65536 /* NOTE: Must be synchronized with ADC_VIRTUAL_SAMPLES */

static inline dsp_num
param_exponential_lerp_factor(uint32_t adc_value) {
	return 0;
}

static inline dsp_num
param_linear(uint32_t adc_value) {
	const dsp_num mul = dsp_one / INPUT_MAX;
	return adc_value * mul;
}

static inline dsp_num
exponent_by_squaring(dsp_num x, uint32_t exponent) {
	if(exponent <= 1) {
		if(exponent == 1) return x;
		if(exponent == 0) return 1;
	}
	
	dsp_num x2 = dsp_mul(x, x);

	if(x & 1) {
		return dsp_mul(x, exponent_by_squaring(x2, (exponent - 1) / 2));
	}

	return exponent_by_squaring(x2, exponent / 2);
}

static inline dsp_num
param_gain(uint32_t adc_value) {
	//const dsp_num small = dsp_from_double(0.9999);
	//return exponent_by_squaring(small, adc_value);

	return dsp_from_double(pow(0.9999, adc_value));
}

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
void audio_params_tick_multiplexer(audio_params *ap);

#endif