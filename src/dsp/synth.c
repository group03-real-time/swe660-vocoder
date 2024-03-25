#include "synth.h"

#include <string.h>

/**
 * We can't store frequencies directly, because our DSP numbers are only in the
 * range -4 to slightly less than 4.
 * 
 * Instead, store the phase offset directly. This does mean that detuning might
 * be a little weird, but it should be possible to implement that using a
 * multiplier on the phase offset.
*/
static dsp_num phase_offset_table[NUMBER_OF_NOTES];

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

void
synth_voice_process(synth_voice *v) {
	v->phase += v->phase_step;
	while(v->phase >= dsp_one) {
		v->phase -= dsp_one;
	}

	/* Multiply the value by a large prime to try to mix the digits */
	v->white_noise_generator = ((v->white_noise_generator + 1) * 12347843);

	/* Keep it within the range -1, 1 for better mixing. */
	const dsp_num white_noise = dsp_rshift(v->white_noise_generator, 2);
	const dsp_num sawtooth = sawtooth_wave(v->phase);

	v->sample = dsp_rshift(sawtooth, 1) + dsp_rshift(white_noise, 3);
	v->envelope = dsp_zero;
	if(v->state != SYNTH_RELEASE) {
		v->envelope = dsp_one;
	}
}

dsp_num
synth_process(synth *syn) {
	dsp_largenum suml = 0;

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

	/* Initialize phase offset table */
	for(int i = 0; i < NUMBER_OF_NOTES; ++i) {
		phase_offset_table[i] = dsp_from_double(freq / SAMPLE_RATE);
		freq *= semitone;
	}

	memset(syn, 0, sizeof(*syn));
	syn->next_age = 1;

	for(int i = 0; i < MAX_SYNTH_VOICES; ++i) {
		/* All voices start out in release state */
		syn->voices[i].state = SYNTH_RELEASE;

		/* Initialize all white noises with different values for "variety" */
		syn->voices[i].white_noise_generator = i;
	}

	
}