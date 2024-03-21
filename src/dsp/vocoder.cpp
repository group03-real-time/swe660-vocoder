#include "vocoder.h"

#include <math.h>
#include <string.h>

#include "../../iir1/iir/Butterworth.h"

struct vocoder {
	Iir::Butterworth::BandPass<4> mod_filters[VOCODER_BANDS];
	Iir::Butterworth::BandPass<4> car_filters[VOCODER_BANDS];
	float envelope_follow[VOCODER_BANDS];

	float mod_ef = 0.0;
	float sum_ef = 0.0;
};

float
vc_process(vocoder *v, float mod, float car) {
	/* TODO: Optimization:
	 * We should only update the x array once for each entire set of bands.
	 * This should save about half of the copying time. */

	float sum = 0.0;

	for(int i = 0; i < VOCODER_BANDS; ++i) {
		float m = v->mod_filters[i].filter<float>(mod);
		/* First, update the eq band for measuring modulator amplitude */
		//eq_update(&v->mod_filters[i], mod);

		/* Then, update the envelope follower. We basically low-pass-filter
		 * the absolute value of the signal. */
		float ef = fabsf(m);
		v->envelope_follow[i] += (ef - v->envelope_follow[i]) * 0.008;

		/* Finally, update each of the carrier filters, and multiply them
		 * by the ef value. */
		//eq_update(&v->carrier_filters[i], car);
		float c = v->car_filters[i].filter<float>(car);

		sum += c * v->envelope_follow[i];

		
	}

	v->mod_ef += (fabsf(mod) - v->mod_ef) * 0.0008;
	v->sum_ef += (fabsf(sum) - v->sum_ef) * 0.0008;

	/* should be < 1 */
	float amp = v->mod_ef;

	if(v->sum_ef != 0.0) {
		amp = v->mod_ef / v->sum_ef;
	}

	return sum * amp;
}

vocoder*
vc_new() {
	vocoder *v = new vocoder();
	
	double min_freq = 0;
	double max_freq = 4000.0 / SAMPLE_RATE;
	double range = (max_freq - min_freq);

	//double total_octaves = log2(SAMPLE_RATE / 2.0);

	double freq_div = range / (VOCODER_BANDS + 1);
	//double bandwidth = total_octaves * freq_div;

	/* Basically: don't create a band at 0 or 0.5, but at evenly spaced
	 * divisions within 0--0.5 */
	double f = freq_div;

	for(int i = 0; i < VOCODER_BANDS; ++i) {
		v->envelope_follow[i] = 0;

		v->mod_filters[i].setupN(f, freq_div);
		v->car_filters[i].setupN(f, freq_div);
		//memset(&v->carrier_filters[i], 0, sizeof(rbj_eq));
		//memset(&v->mod_filters[i], 0, sizeof(rbj_eq));

		//double frac_bw = (2.0 * freq_div) / f;

		//eq_create_bpf(&v->carrier_filters[i], f, f / (2.0 * freq_div));
		//eq_create_bpf(&v->mod_filters[i], f, f / (2.0 * freq_div));

		f += freq_div;
	}

	return v;
}