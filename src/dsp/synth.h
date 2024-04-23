#ifndef SYNTH_H
#define SYNTH_H

#include "dsp.h"
#include "audio_params.h"

/** The maximum number of voices impacts the performance of the synth. */
#define MAX_SYNTH_VOICES 4

/** 
 * The number of notes mainly impacts the size of the array of note frequency
 * data. 
 */
#define NUMBER_OF_NOTES 64

/**
 * Defines the states used in the ADSR state machine for the synthesizer.
 */
typedef enum {
	SYNTH_ATTACK,
	SYNTH_DECAY,
	SYNTH_SUSTAIN,
	SYNTH_RELEASE
} synth_envelope_state;

typedef struct {
	/* Which note this synth is playing. */
	int note;

	/* Used for voice-stealing */
	uint32_t age;

	/* The most recently computed sample */
	dsp_num sample;

	/* The computed ADSR envelope of the synth */
	dsp_num envelope;

	/* Internal state used for generating white noise */
	dsp_num white_noise_generator;

	/* The phase of the oscillator */
	dsp_num phase;
	/* How much the phase is incremented per sample. Corresponds to note frequency. */
	dsp_num phase_step;

	/* The state of the voice's ADSR. */
	synth_envelope_state state;
} synth_voice;

/**
 * Defines the state for a synthesizer. Similar to the vocoder, this should 
 * generally be statically allocated somewhere for efficiency.
 */
typedef struct {
	/* The next age value to assign to a voice, for voice-stealing. */
	uint32_t next_age;

	/* The array of all synth voices. */
	synth_voice voices[MAX_SYNTH_VOICES];
} synth;

/**
 * Initializes the synthesizer with the necessary state to start playing notes.
 */
void synth_init(synth *syn);

/**
 * Presses a new note on the synthesizer. Will steal a voice if necessary.
 * 
 * Note that, at the moment, only one copy of a given note should ever be played
 * at a time -- that is, synth_press(..., n) should not be called twice without
 * an intermediate synth_release(..., n). This works perfectly fine with the
 * existing button keyboard design.
 */
void synth_press(synth *syn, int note);

/**
 * Releases the given note on the synthesizer. Causes the associated voice to
 * go into SYNTH_RELEASE state.
 */
void synth_release(synth *syn, int note);

/**
 * Computes the next sample for the given synth, given the specified audio
 * parameters.
 */
dsp_num synth_process(synth *syn, audio_params *ap);

/**
 * Debugging method: Prints out the notes that are currently active on the
 * synthesizer.
 */
void synth_print_active_notes(synth *syn);

#endif