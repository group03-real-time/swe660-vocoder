#include "synth.h"

#include <string.h>

static dsp_num frequency_table[NUMBER_OF_NOTES];

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
	dsp_num freq = frequency_table[note];
	syn->voices[idx].state = SYNTH_ATTACK;
	syn->voices[idx].age = syn->next_age;
	syn->voices[idx].phase_step = dsp_div_int_denom(freq, SAMPLE_RATE);

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

void
synth_voice_process(synth_voice *v) {
	v->phase += v->phase_step;
	while(v->phase >= dsp_one) {
		v->phase -= dsp_one;
	}

	v->sample = sawtooth_wave(v->phase);
	v->envelope = dsp_zero;
	if(v->state != SYNTH_RELEASE) {
		v->envelope = dsp_one;
	}
}

dsp_num
synth_process(synth *syn) {
	dsp_largenum suml = dsp_zero;

	for(int i = 0; i < MAX_SYNTH_VOICES; ++i) {
		synth_voice *v = &syn->voices[i];
		synth_voice_process(v);

		suml += dsp_mul_large(v->sample, v->envelope);
	}

	return dsp_compact(suml);
}

void
synth_init(synth *syn) {
	const double semitone = 1.05946309435929526456182529494634170077920431749418;

	/* Start at A2? */
	double freq = 110;

	/* Initialize frequency table */
	for(int i = 0; i < NUMBER_OF_NOTES; ++i) {
		frequency_table[i] = dsp_from_double(freq);
		freq *= semitone;
	}

	memset(syn, 0, sizeof(*syn));
	syn->next_age = 1;

	for(int i = 0; i < MAX_SYNTH_VOICES; ++i) {
		/* All voices start out in release state */
		syn->voices[i].state = SYNTH_RELEASE;
	}
}