#include "synth.h"

#include <string.h>
#include <math.h>

/**
 * We can't store frequencies directly, because our DSP numbers are only in the
 * range -4 to slightly less than 4.
 * 
 * Instead, store the phase offset directly. This does mean that detuning might
 * be a little weird, but it should be possible to implement that using a
 * multiplier on the phase offset.
*/
static dsp_num phase_offset_table[NUMBER_OF_NOTES];

#define SINC_SIZE 15
#define SINC_PHASE_START 0.3
#define SINC_PHASE_STEP  0.2

/* sinc table should consist of premultiplied sinc(x) * step_size (for riemann sum) */
static dsp_num sinc_table[SINC_SIZE];
static dsp_num sinc_table_step;
static dsp_num sinc_first_step;

void
synth_press(synth *syn, int note) {
	/* First, steal a voice. */
	int idx = 0;
	uint32_t age = syn->voices[0].age;

	/* Find the voice that has least recently been stolen. */
	for(int i = 1; i < MAX_SYNTH_VOICES; ++i) {
		if(syn->voices[i].age < age) {
			age = syn->voices[i].age;
			idx = i;
		}
	}

	/* Set state and frequency. */
	syn->voices[idx].state = SYNTH_ATTACK;
	syn->voices[idx].envelope = dsp_zero; /* The envelope must reset to 0 */
	syn->voices[idx].age = syn->next_age;
	syn->voices[idx].phase_step = phase_offset_table[note];

	/* Track the note */
	syn->voices[idx].note = note;

	/* Track next age value */
	syn->next_age += 1;
}

void
synth_release(synth *syn, int note) {
	for(int i = 0; i < MAX_SYNTH_VOICES; ++i) {
		/* Assumption: Only one voice is ever playing a given note at a time 
		 * (Potential TODO: Make this look for the "most recent" note?) */
		if(syn->voices[i].note == note) {
			syn->voices[i].state = SYNTH_RELEASE;
		}
	}
}

/** This is not a bandlimited function. */
static inline dsp_num
sawtooth_wave(dsp_num phase) {
	/* The basic sawtooth shape is 1 - 2x, which can be efficiently implemented
	 * with a bitshift. */
	dsp_num result = dsp_one - dsp_lshift(phase, 1);

	/* For now, scale the result for testing. */
	return dsp_rshift(result, 2);
}

static inline dsp_num
square_wave(dsp_num phase) {
	if(phase > (dsp_one / 2)) {
		return -dsp_rshift(dsp_one, 2);
	}
	return dsp_rshift(dsp_one, 2);
}

static inline dsp_num
phase_small_increment(dsp_num in, dsp_num step) {
	in += step;
	if(in > dsp_one) {
		in -= dsp_one;
	}
	return in;
}

static inline dsp_num
phase_small_decrement(dsp_num in, dsp_num step) {
	in -= step;
	if(in < dsp_zero) {
		in += dsp_one;
	}
	return in;
}


static inline dsp_num
voice_compute_waveform(synth_voice *v) {
	const dsp_num total_step = v->phase_step;
	const dsp_num first_step = dsp_mul(total_step, sinc_first_step);
	const dsp_num step       = dsp_mul(total_step, sinc_table_step);

	dsp_num phase_pos = phase_small_increment(v->phase, first_step);
	dsp_num phase_neg = phase_small_decrement(v->phase, first_step);

	/* The sum includes the center sample with weight 1 */
	dsp_largenum suml = sawtooth_wave(v->phase);

	for(int i = 0; i < SINC_SIZE; ++i) {
		dsp_num sample1 = sawtooth_wave(phase_pos);
		dsp_num sample2 = sawtooth_wave(phase_neg);

		suml += dsp_mul_large(sample1 + sample2, sinc_table[i]);

		phase_pos = phase_small_increment(phase_pos, step);
		phase_neg = phase_small_decrement(phase_neg, step);
	}

	dsp_num sum = dsp_compact(suml);
	return sum;//dsp_div(sum, sinc_weight);
}

void
synth_voice_process(synth_voice *v, audio_params *ap) {
	v->phase += v->phase_step;
	while(v->phase >= dsp_one) {
		v->phase -= dsp_one;
	}

	/* Multiply the value by a large prime to try to mix the digits */
	v->white_noise_generator = ((v->white_noise_generator + 1) * 12347843);

	/* Keep it within the range -1, 1 for better mixing. */
	const dsp_num white_noise = dsp_rshift(v->white_noise_generator, 2);
	const dsp_num sawtooth = voice_compute_waveform(v);

	v->sample
		= dsp_rshift(sawtooth, 1)
		+ dsp_mul(white_noise, ap->noise_gain);

	/* Update envelope */
	const dsp_num small_difference = dsp_from_double(0.02);

	if(v->state == SYNTH_ATTACK) {
		/* Ramp up from 0 to 1 */
		dsp_num dif = dsp_one - v->envelope;
		v->envelope += dsp_mul(dif, ap->attack);
		if(dsp_abs(dif) <= small_difference) {
			v->envelope = dsp_one;
			v->state = SYNTH_DECAY;
		}
	}

	if(v->state == SYNTH_DECAY) {
		dsp_num dif = ap->sustain - v->envelope;
		v->envelope += dsp_mul(dif, ap->decay);
		if(dsp_abs(dif) <= small_difference) {
			v->envelope = ap->sustain;
			v->state = SYNTH_SUSTAIN;
		}
	}

	if(v->state == SYNTH_RELEASE) {
		dsp_num dif = dsp_zero - v->envelope;
		v->envelope += dsp_mul(dif, ap->release);
		if(dsp_abs(dif) <= small_difference) {
			v->envelope = dsp_zero;
			v->state = SYNTH_SUSTAIN;
		}
	}
}

dsp_num
synth_process(synth *syn, audio_params *ap) {
	dsp_largenum suml = 0;

	for(int i = 0; i < MAX_SYNTH_VOICES; ++i) {
		synth_voice *v = &syn->voices[i];
		synth_voice_process(v, ap);

		suml += dsp_mul_large(v->sample, v->envelope);
	}

	return dsp_compact(suml);
}

double
sinc_eval(double x) {
	const double pi = 3.14159265358979323846264;
	return sin(x * pi) / (x * pi);
}

void
synth_init(synth *syn) {
	const double semitone = 1.05946309435929526456182529494634170077920431749418;

	/* Start at A2? */
	double freq = 110;

	/* Initialize phase offset table */
	for(int i = 0; i < NUMBER_OF_NOTES; ++i) {
		phase_offset_table[i] = dsp_from_double(freq / SAMPLE_RATE);
		freq *= semitone;
	}

	/* Compute sinc table */
	double phase = SINC_PHASE_START;

	/* first step: off from the x = 0 */
	sinc_first_step = dsp_from_double(phase);
	double step = SINC_PHASE_STEP;
	for(int i = 0; i < SINC_SIZE; ++i) {
		double y = sinc_eval(phase);
		double w = step; /* riemann sum: areas of rectangles, precompute this multiply */
		sinc_table[i] = dsp_from_double(y * w);
		phase += step;
	}
	
	sinc_table_step = dsp_from_double(step);

	memset(syn, 0, sizeof(*syn));
	syn->next_age = 1;

	for(int i = 0; i < MAX_SYNTH_VOICES; ++i) {
		/* All voices start out in release state */
		syn->voices[i].state = SYNTH_RELEASE;

		/* Initialize all white noises with different values for "variety" */
		syn->voices[i].white_noise_generator = i;
	}

	
}