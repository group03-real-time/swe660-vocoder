#ifndef SYNTH_H
#define SYNTH_H

#include "dsp.h"
#include "audio_params.h"

#define MAX_SYNTH_VOICES 4
#define NUMBER_OF_NOTES 64

typedef enum {
	SYNTH_ATTACK,
	SYNTH_DECAY,
	SYNTH_SUSTAIN,
	SYNTH_RELEASE
} synth_envelope_state;

typedef struct {
	int note;

	/* Used for voice-stealing */
	uint32_t age;

	dsp_num sample;
	dsp_num envelope;

	dsp_num white_noise_generator;

	dsp_num phase;
	dsp_num phase_step;

	synth_envelope_state state;
} synth_voice;

typedef struct {
	uint32_t next_age;
	synth_voice voices[MAX_SYNTH_VOICES];
} synth;

void synth_init(synth *syn);

void synth_press(synth *syn, int note);
void synth_release(synth *syn, int note);

dsp_num synth_process(synth *syn, audio_params *ap);

#endif