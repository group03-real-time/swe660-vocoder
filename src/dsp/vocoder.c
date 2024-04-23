#include "vocoder.h"

#include <math.h>
#include <string.h>

/* We include the actual implementation code for the BPF filters in our vocoder
 * c file. This is to give the compiler the ability to inline more code and
 * optimize more. Results in a ~12% speedup. */
#include "bpf_impl.c"

dsp_num
vc_process(vocoder *v, dsp_num mod, dsp_num car) {
	const dsp_num lerp_factor_ef    = dsp_from_double(0.008);
	const dsp_num lerp_factor_bigef = dsp_from_double(0.0008);

	const dsp_num lerp_factor_in = dsp_from_double(0.08);

	memmove(v->mod_x + 1, v->mod_x, sizeof(dsp_num) * 2);
	memmove(v->car_x + 1, v->car_x, sizeof(dsp_num) * 2);

	dsp_num mod_in = mod * INPUT_EXTRA_MUL;
	dsp_num car_in = car * INPUT_EXTRA_MUL;
	v->mod_lowpass += dsp_mul((mod_in - v->mod_lowpass), lerp_factor_in);

	v->car_lowpass += dsp_mul((car_in - v->car_lowpass), lerp_factor_in);

	v->mod_x[0] = v->mod_lowpass;
	v->car_x[0] = v->car_lowpass;

	dsp_largenum suml = dsp_zero;

	for(int i = 0; i < VOCODER_BANDS; ++i) {
		dsp_num m = bpf_cbq_update(&v->mod_filters[i], v->mod_x);
		/* First, update the eq band for measuring modulator amplitude */

		/* Then, update the envelope follower. We basically low-pass-filter
		 * the absolute value of the signal. */
		dsp_num ef = dsp_abs(m);
		v->envelope_follow[i] += dsp_mul((ef - v->envelope_follow[i]), lerp_factor_ef);

		/* Finally, update each of the carrier filters, and multiply them
		 * by the ef value. */
		dsp_num c = bpf_cbq_update(&v->car_filters[i], v->car_x);

		suml += dsp_mul_large(c, v->envelope_follow[i]);
	}

	dsp_num sum = dsp_compact(suml);

	v->mod_ef += dsp_mul((dsp_abs(mod) - v->mod_ef), lerp_factor_bigef);
	v->sum_ef += dsp_mul((dsp_abs(sum) - v->sum_ef), lerp_factor_bigef);

	/* Note: The overall amplification seems to mostly just make the software
	 * behave worse on the actual hardware setup. SO, it is commented out
	 * for now. */

	/* should be < 1 */
	//dsp_num amp = 0;

	//if(v->sum_ef != 0) {
	//	amp = dsp_div(v->mod_ef, v->sum_ef);
	//}
	
	/* If the sum is 0, that means there's no carrier signal: so don't have
	 * any output signal either. */

	return sum;////dsp_mul(sum, amp);
}

void
vc_init(vocoder *v) {
	memset(v, 0, sizeof(*v));
	
	double min_freq = 0;
	double max_freq = 8000.0 / SAMPLE_RATE;
	double range = (max_freq - min_freq);

	double freq_div = range / (VOCODER_BANDS + 1);

	/* Basically: don't create a band at 0 or 0.5, but at evenly spaced
	 * divisions within 0--0.5 */
	double f = freq_div;

	for(int i = 0; i < VOCODER_BANDS; ++i) {
		v->envelope_follow[i] = 0;

		design_bpf(&v->mod_filters[i], f, freq_div);
		design_bpf(&v->car_filters[i], f, freq_div);

		f += freq_div;
	}
}