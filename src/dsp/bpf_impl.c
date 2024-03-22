#include "bpf.h"

/* We provide three biquad update functions: even, odd, and scaled_even.
 * 
 * This is because the b0, b1, and b2 coefficients always take particular forms
 * for the bandpass.
 * 
 * The b0 and b2 coefficients are always 1, so we simply don't multiply them in.
 * (and don't store them).
 * 
 * The b1 coefficient is either 2 (for even indices) or -2 (for odd indices).
 * So, we simply provide two versions of the function, one for even and one
 * for odd, that perform either the x2 or the x-2.
 * 
 * Finally, the first filter in the chain is supposed to have its b coefficients
 * scaled by some value. This scaling is better to apply separately in the fixed
 * point math for more precision, and also only needs to be applied once, so
 * again we provide another function.
 * 
 * Besides performing a Direct Form I update, each of these functions also copies
 * the old Y values over. We assume the X values were copied previously.
 * 
 * Finally, we also provide an input_gain to be multiplied to the x array. This
 * gain value should not be a fixed point number--it should be a direct multiplication
 * for speed (ideally a power of two).
*/
static inline void 
bpf_bq_update_even(bpf_biquad *bq, dsp_num *x, dsp_num *y, int input_gain) {
	memmove(y + 1, y, sizeof(*y) * 2);

	y[0] = (x[0] * input_gain)
	     + dsp_lshift(x[1] * input_gain, 1) /* even index: b1 = 2 */
		 + (x[2] * input_gain)
		 - dsp_mul(bq->a1, y[1])
		 - dsp_mul(bq->a2, y[2]);

#ifdef DSP_FLOAT
	/* Flush denormalized values for 11x speed improvement on x86 */
	if(dsp_abs(y[0]) < 1.175494350822287508e-38) {
		y[0] = 0.0;
	}
#endif
}

static inline void 
bpf_bq_update_odd(bpf_biquad *bq, dsp_num *x, dsp_num *y, int input_gain) {
	memmove(y + 1, y, sizeof(*y) * 2);

	y[0] = (x[0] * input_gain)
	     - dsp_lshift(x[1] * input_gain, 1) /* odd index: b1 = -2 */
		 + (x[2] * input_gain)
		 - dsp_mul(bq->a1, y[1])
		 - dsp_mul(bq->a2, y[2]);

#ifdef DSP_FLOAT
	/* Flush denormalized values for 11x speed improvement on x86 */
	if(dsp_abs(y[0]) < 1.175494350822287508e-38) {
		y[0] = 0.0;
	}
#endif
}

static inline void 
bpf_bq_update_scaled_even(bpf_biquad *bq, dsp_num *x, dsp_num *y, dsp_num scale) {
	memmove(y + 1, y, sizeof(*y) * 2);

	y[0] = dsp_mul(x[0], scale)
	     + dsp_mul(dsp_lshift(x[1], 1), scale) /* even index: b1 = 2 */
		 + dsp_mul(x[2], scale)
		 - dsp_mul(bq->a1, y[1])
		 - dsp_mul(bq->a2, y[2]);

#ifdef DSP_FLOAT
	/* Flush denormalized values for 11x speed improvement on x86 */
	if(dsp_abs(y[0]) < 1.175494350822287508e-38) {
		y[0] = 0.0;
	}
#endif
}

static const int
bpf_input_gains[] = {
	1, 2, 2, 2, 2, 2, 2, 2
};

/* Assume x was already updated */
static inline dsp_num
bpf_cbq_update(bpf_cascaded_biquad *bq, dsp_num *x) {
	/* First biquad in the chain is scaled. */
	bpf_bq_update_scaled_even(&bq->biquads[0], x, bq->y_array[0], bq->scale);

	/* Note: NUM_STAGES must be at least 2 */
	bpf_bq_update_odd(&bq->biquads[1],
			bq->y_array[0],
			bq->y_array[1], bpf_input_gains[1]);

	/* Update the rest of the stages using the normal update functions. */
	for(int i = 2; i < NUM_STAGES; i += 2) {
		bpf_bq_update_even(&bq->biquads[i],
			bq->y_array[i - 1],
			bq->y_array[i], bpf_input_gains[i]);
		bpf_bq_update_odd(&bq->biquads[i + 1],
			bq->y_array[i + 1 - 1],
			bq->y_array[i + 1], bpf_input_gains[i + 1]);
	}
	
	/* The result is in the last stage y[0]. */
	return bq->y_array[NUM_STAGES - 1][0];
}
