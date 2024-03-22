#ifndef BPF_H
#define BPF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dsp.h"

typedef struct {
	/* All coefficients normalized by a0 */

	/* We do not provide b0, b1, or b2 coefficients for the bpf biquad. */
	/* dsp_num b0, b1, b2; */

	/* Note: b1 is either -2 or 2, and in fact, this is just a matter of whether
	 * the biquad is at at even index or odd index. (even index = positive 2, 
	 * odd index = -2.) So instead of storing that, we just use two functions, 
	 * and hope the compiler figures out the best thing to do. */

	dsp_num a1;
	dsp_num a2;
} bpf_biquad;

/* Note: NUM_STAGES *must* be even */
#define NUM_STAGES 4

typedef struct {
	bpf_biquad biquads[NUM_STAGES];
	dsp_num    y_array[NUM_STAGES][3];
	dsp_num    scale;
} bpf_cascaded_biquad;

void design_bpf(bpf_cascaded_biquad *cbq, double fc, double fw);

#endif