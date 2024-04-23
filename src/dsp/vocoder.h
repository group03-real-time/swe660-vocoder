#ifndef VOCODER_H
#define VOCODER_H

#include "dsp.h"
#include "bpf.h"

/**
 * The vocoder struct. Contains all the state needed to perform the vocoding
 * over time (because IIR filters are stateful).
 * 
 * Should generally be stack-allocated or otherwise statically allocated for
 * efficiency.
 */
typedef struct {
	/** The BPFs for the modulator signal. */
	bpf_cascaded_biquad mod_filters[VOCODER_BANDS];
	/** The BPFs for the carrier signal. */
	bpf_cascaded_biquad car_filters[VOCODER_BANDS];
	/** The envelope followers for each filtered modulator signal. */
	dsp_num envelope_follow[VOCODER_BANDS];

	/* Slightly low pass the modulator using a lerp */
	dsp_num mod_lowpass;

	dsp_num car_lowpass;

	/**
	 * The input array for the filters. This is used to keep the filter chain
	 * efficient (the input to each one is the output of the previous), but
	 * someone needs to store the top-level input -- so it's stored here.
	 */
	dsp_num mod_x[3];
	dsp_num car_x[3];

	/** 
	 * Envelope followers for the overall signal. Used to make the overall
	 * gain of the carrier vaguely match the modulator.
	 */
	dsp_num mod_ef;
	dsp_num sum_ef;
} vocoder;

/**
 * Initializes the vocoder with all the necessary state for it to process
 * a signal through vc_process.
 */
void vc_init(vocoder *v);

/**
 * Computes a single sample run through the vocoder. Requires an input for both
 * the modulator signal and the carrier signal. Returns the vocoded signal.
 */
dsp_num vc_process(vocoder *v, dsp_num modulator, dsp_num carrier);

#endif