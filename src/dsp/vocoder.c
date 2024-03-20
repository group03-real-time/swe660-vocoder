#include "vocoder.h"

#include <math.h>
#include <string.h>

float
vc_process(vocoder *v, float mod, float car) {
	/* TODO: Optimization:
	 * We should only update the x array once for each entire set of bands.
	 * This should save about half of the copying time. */

	float sum = 0.0;

	for(int i = 0; i < VOCODER_BANDS; ++i) {
		/* First, update the eq band for measuring modulator amplitude */
		eq_update(&v->mod_filters[i], mod);

		/* Then, update the envelope follower. We basically low-pass-filter
		 * the absolute value of the signal. */
		float ef = fabsf(v->mod_filters[i].y[0]);
		v->envelope_follow[i] += (ef - v->envelope_follow[i]) * 0.07;

		/* Finally, update each of the carrier filters, and multiply them
		 * by the ef value. */
		eq_update(&v->carrier_filters[i], car);

		sum += v->carrier_filters[i].y[0] * v->envelope_follow[i];
	}

	return sum;
}

void
vc_init(vocoder *v) {
	double freq_div = 0.5 / (VOCODER_BANDS + 1);
	double bandwidth = freq_div;

	/* Basically: don't create a band at 0 or 0.5, but at evenly spaced
	 * divisions within 0--0.5 */
	double f = freq_div;

	for(int i = 0; i < VOCODER_BANDS; ++i) {
		memset(&v->carrier_filters[i], 0, sizeof(rbj_eq));
		memset(&v->mod_filters[i], 0, sizeof(rbj_eq));

		eq_create_bpf(&v->carrier_filters[i], f, bandwidth);
		eq_create_bpf(&v->mod_filters[i], f, bandwidth);

		f += freq_div;
	}
}