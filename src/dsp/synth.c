#include "synth.h"

#include <string.h>
#include <math.h>
#include <stdio.h>

/**
 * We can't store frequencies directly, because our DSP numbers are only in the
 * range -4 to slightly less than 4.
 * 
 * Instead, store the phase offset directly. This does mean that detuning might
 * be a little weird, but it should be possible to implement that using a
 * multiplier on the phase offset.
*/
static dsp_num phase_offset_table[NUMBER_OF_NOTES];

#define SINC_SIZE 5
#define SINC_PHASE_START 0.4
#define SINC_PHASE_STEP  0.4

/* sinc table should consist of premultiplied sinc(x) * step_size (for riemann sum) */
static dsp_num sinc_table[SINC_SIZE];
static dsp_num sinc_table_step;
static dsp_num sinc_first_step;

void
synth_press(synth *syn, int note) {
	int idx = 0;

	/* First, if there is a voice that is at envelope gain 0 (and in release or sustain),
	 * then we can simply steal that voice. */
	for(int i = 0; i < MAX_SYNTH_VOICES; ++i) {
		if(syn->voices[i].state == SYNTH_RELEASE || syn->voices[i].state == SYNTH_SUSTAIN) {
			if(syn->voices[i].envelope == 0) {
				idx = i;
				goto have_a_voice;
			}
		}
	}

	/* Otherwise, steal an active voice. Init to 0 then check the other ones. */
	idx = 0;
	uint32_t age = syn->voices[0].age;

	/* Find the voice that has least recently been stolen. */
	for(int i = 1; i < MAX_SYNTH_VOICES; ++i) {
		if(syn->voices[i].age < age) {
			age = syn->voices[i].age;
			idx = i;
		}
	}

have_a_voice:
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

static bool
synth_voice_active(synth_voice *v) {
	if(v->state == SYNTH_ATTACK || v->state == SYNTH_DECAY) return false;
	return v->envelope != 0;
}

void
synth_print_active_notes(synth *syn) {
	printf("active notes: ");
	for(int i = 0; i < MAX_SYNTH_VOICES; ++i) {
		if(synth_voice_active(&syn->voices[i])) {
			printf("%d (on voice %d) ", syn->voices[i].note, i);
		}
	}
	puts("");
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
voice_sample_waveform(dsp_num phase, audio_params *ap) {
	dsp_num saw = sawtooth_wave(phase);
	dsp_num square = square_wave(phase);

	return dsp_mul(square, ap->wave_shape) + dsp_mul(saw, (dsp_one - ap->wave_shape));
}

static inline dsp_num
voice_compute_waveform(synth_voice *v, audio_params *ap) {
	const dsp_num total_step = v->phase_step;
	const dsp_num first_step = dsp_mul(total_step, sinc_first_step);
	const dsp_num step       = dsp_mul(total_step, sinc_table_step);

	dsp_num phase_pos = phase_small_increment(v->phase, first_step);
	dsp_num phase_neg = phase_small_decrement(v->phase, first_step);

	/* The sum includes the center sample with weight 1 */

	dsp_largenum suml = 0;
	int odd = (SINC_SIZE & 1);
	{
		dsp_num middle = voice_sample_waveform(v->phase, ap);

		/* The middle sample is multiplied by 4 if we have odd count, by 2 if 
		 * we have even count */
		middle = odd ? dsp_lshift(middle, 2) : dsp_lshift(middle, 1);
		suml = middle;
	}

	for(int i = 0; i < SINC_SIZE; ++i) {
		odd = !odd;
		int shift = 1 + odd;

		dsp_num sample1 = voice_sample_waveform(phase_pos, ap);
		dsp_num sample2 = voice_sample_waveform(phase_neg, ap);

		/* The last sample is the endpoints and is not shifted. */
		if(i < SINC_SIZE - 1) {
			sample1 = dsp_lshift(sample1, shift);
			sample2 = dsp_lshift(sample2, shift);
		}

		suml += dsp_mul_large(sample1 + sample2, sinc_table[i]);

		phase_pos = phase_small_increment(phase_pos, step);
		phase_neg = phase_small_decrement(phase_neg, step);
	}

	dsp_num sum = dsp_compact(suml);
	return sum;//dsp_div(sum, sinc_weight);
}

void
synth_voice_process(synth_voice *v, audio_params *ap) {
	v->phase += dsp_mul(v->phase_step, ap->tuning);
	while(v->phase >= dsp_one) {
		v->phase -= dsp_one;
	}

	/* Multiply the value by a large prime to try to mix the digits */
	v->white_noise_generator = ((v->white_noise_generator + 1) * 12347843);

	/* Keep it within the range -1, 1 for better mixing. */
	const dsp_num white_noise = dsp_rshift(v->white_noise_generator, 2);
	const dsp_num sawtooth = voice_compute_waveform(v, ap);

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
	double phase = SINC_PHASE_STEP;

	/* first step: off from the x = 0 */
	sinc_first_step = dsp_from_double(phase);
	double step = SINC_PHASE_STEP;
	for(int i = 0; i < SINC_SIZE; ++i) {
		double y = sinc_eval(phase);
		double w = step; /* composite simpson's rule: includes a 1/3 h factor out front (h is the step size) */
		sinc_table[i] = dsp_from_double(y * w / 3.0);
		phase += step;
	}
	
	sinc_table_step = dsp_from_double(step);

	memset(syn, 0, sizeof(*syn));
	syn->next_age = 1;

	for(int i = 0; i < MAX_SYNTH_VOICES; ++i) {
		/* All voices start out in release state */
		syn->voices[i].state = SYNTH_RELEASE;

		syn->voices[i].envelope = 0;

		/* Initialize all white noises with different values for "variety" */
		syn->voices[i].white_noise_generator = i;
	}

	
}