#include "bpf.h"

#include <string.h>
#include <math.h>
#include <complex.h>

#include "app.h"

typedef struct {
	double a1;
	double a2;
	double b0;
	double b1;
	double b2;
} double_biquad;

/**
 * Important note:
 * The code for doing the filter design for the Butterworth bandpass filter is
 * ported directly from https://github.com/berndporr/iir1. Our implementation
 * speeds up the actual DSP from this library by a factor of 1.5x on x86.
 * 
 * The filter design code is largely unchanged, besides that it is ported
 * from C++ to C, and that the scale factor was moved out of the b coefficients
 * and into a separate value.
 * 
 * This library is MIT licensed.
*/

/* --- Various helper data structures --- */

typedef struct {
	complex p1;
	complex z1;
	complex p2;
	complex z2;
} pole_zero_pair;

typedef struct {
	complex first;
	complex second;
} complex_pair;

typedef struct {
	pole_zero_pair poles[NUM_STAGES / 2];
	double w;
	double gain;
} analog_layout;

typedef struct {
	pole_zero_pair poles[NUM_STAGES];
	double w;
	double gain;
} digital_layout;

#define COMPLEX_PAIR(a, b) (complex_pair){.first = a, .second = b}
#define POLE_ZERO_PAIR_CONJ(pole, zero)\
(pole_zero_pair){\
	.p1 = pole,\
	.z1 = zero,\
	.p2 = conj(pole),\
	.z2 = conj(zero)\
}

static const double pi	  = 3.1415926535897932384626433832795028841971;
static const double pi2  = 1.5707963267948966192313216916397514420986;

/* --- Helper functions for complex math --- */
static inline
complex polar(double r, double theta) {
	return r * (cos(theta) + I * sin(theta));
}

static inline
complex addmul(complex a, double v, complex b) {
	 return (creal(a) + v * creal(b)) + I * (cimag(a) + v * cimag(b));
}

static inline double
norm(complex c) {
	double i = cimag(c);
	double r = creal(c);
	return i * i + r * r;
}

static void
analog_design(analog_layout *analog) {
	/* Appears to perform the analog filter design. */
	const double n2 = 2 * NUM_STAGES;
	const int pairs = NUM_STAGES / 2;
	for (int i = 0; i < pairs; ++i)
	{
		complex pole = polar(1., pi2 + (2 * i + 1) * pi / n2);
		complex zero = INFINITY;

		analog->poles[i] = POLE_ZERO_PAIR_CONJ(pole, zero);
	}

	analog->w    = 0.0;
	analog->gain = 1.0;
}

static complex_pair
bp_transform_pair(complex c, double b, double a2, double b2, double ab_2) {
	if (creal(c) == INFINITY)
		return COMPLEX_PAIR(-1, 1);
	
	c = (1. + c) / (1. - c); /* bilinear transform */
	
	complex v = 0;
	v = addmul (v, 4 * (b2 * (a2 - 1) + 1), c);
	v += 8 * (b2 * (a2 - 1) - 1);
	v *= c;
	v += 4 * (b2 * (a2 - 1) + 1);
	v = csqrt(v);
	
	complex u = -v;
	u = addmul (u, ab_2, c);
	u += ab_2;
	
	v = addmul (v, ab_2, c);
	v += ab_2;
	
	complex d = 0;
	d = addmul (d, 2 * (b - 1), c) + 2 * (1 + b);
	
	return COMPLEX_PAIR (u/d, v/d);
}

static void
band_pass_transform(analog_layout *analog, digital_layout *digital, double fc, double fw) {
	if (!(fc < 0.5)) app_fatal_error("filter design bug: fc must be < 0.5");
	if (fc < 0.0)    app_fatal_error("filter design bug: fc must be >= 0.0");
	
	const double ww = 2 * pi * fw;

	double wc2 = 2 * pi * fc - (ww / 2);
	double wc  = wc2 + ww;
	
	/* Apparently the original source performs some clamping for very close
	 * values to 0 and pi. */
	if (wc2 < 1e-8)
		wc2 = 1e-8;
	if (wc  > pi-1e-8)
		wc  = pi-1e-8;
	
	double a =     cos ((wc + wc2) * 0.5) /
		cos ((wc - wc2) * 0.5);
	double b = 1 / tan ((wc - wc2) * 0.5);
	double a2 = a * a;
	double b2 = b * b;
	double ab = a * b;
	double ab_2 = 2 * ab;
	
	const int numPoles = NUM_STAGES;
	const int pairs = numPoles / 2;
	for (int i = 0; i < pairs; ++i)
	{
		complex_pair p1 = bp_transform_pair(analog->poles[i].p1, b, a2, b2, ab_2);
		complex_pair z1 = bp_transform_pair(analog->poles[i].z1, b, a2, b2, ab_2);

		digital->poles[i * 2]     = POLE_ZERO_PAIR_CONJ(p1.first, z1.first);
		digital->poles[i * 2 + 1] = POLE_ZERO_PAIR_CONJ(p1.second, z1.second);
	}
	
	double wn = analog->w;
	digital->w = 2 * atan (sqrt (tan ((wc + wn)* 0.5) * tan((wc2 + wn)* 0.5)));
	digital->gain = analog->gain;
}

static void
bq_set_coefficients(double_biquad *bq, double a0, double a1, double a2, double b0, double b1, double b2) {
	bq->a1 = a1 / a0;
	bq->a2 = a2 / a0;
	bq->b0 = b0 / a0;
	bq->b1 = b1 / a0;
	bq->b2 = b2 / a0;
}

static void
bq_from_pzp(double_biquad *bq, pole_zero_pair *pzp) {
	/* This function computes the coefficients for a biquad based on a pole
	 * zero pair. This is helpful for us, as biquads are at least something
	 * we have some experience with. */

	const double a0 = 1;
	double a1;
	double a2;

	if (cimag(pzp->p1) != 0)
	{
		if (pzp->p2 != conj(pzp->p1)) {
			app_fatal_error("filter design bug: poles must be 0 or conjugate");
		}
		a1 = -2 * creal(pzp->p1);
		a2 = norm(pzp->p1);
	}
	else
	{
		if (cimag(pzp->p2) != 0.0) {
			app_fatal_error("filter design bug: poles must be 0 or conjugate");
		}
		a1 = -(creal(pzp->p1) + creal(pzp->p2));
		a2 =   creal(pzp->p1) * creal(pzp->p2);
	}

	const double b0 = 1;
	double b1;
	double b2;

	if (cimag(pzp->z1) != 0)
	{
		if (pzp->z2 != conj(pzp->z1)) {
			app_fatal_error("filter design bug: zeroes must be 0 or conjugate");
		}
		b1 = -2 * creal(pzp->z1);
		b2 = norm(pzp->z1);
	}
	else
	{
		if (cimag(pzp->z2) != 0.0) {
			app_fatal_error("filter design bug: zeroes must be 0 or conjugate");
		}
		b1 = -(creal(pzp->z1) + creal(pzp->z2));
		b2 =   creal(pzp->z1) * creal(pzp->z2);
	}

	bq_set_coefficients(bq, a0, a1, a2, b0, b1, b2);
}

static complex
cbq_response(bpf_cascaded_biquad *cbq, double_biquad *dbqs, double normalized_frequency) {
	if(normalized_frequency > 0.5) app_fatal_error("filter design bug: normalized_frequency must be <= 0.5");
	if(normalized_frequency < 0.0) app_fatal_error("filter design bug: normalized_frequency must be >= 0.0");

	double w = 2 * pi * normalized_frequency;
	const complex czn1 = polar(1., -w);
	const complex czn2 = polar(1., -2 * w);
	complex ch = 1.0;
	complex cbot = 1.0;

	for (int i = 0; i < NUM_STAGES; ++i) {
		double_biquad *bq = &dbqs[i];

		complex cb = 1.0;
		complex ct =    bq->b0;
		/* Note: original source divides by a0 here, but we guarantee that a0
		 * will always be 1.0 */
		ct = addmul (ct, bq->b1, czn1);
		ct = addmul (ct, bq->b2, czn2);
		cb = addmul (cb, bq->a1, czn1);
		cb = addmul (cb, bq->a2, czn2);
		ch   *= ct;
		cbot *= cb;
	}

	return ch / cbot;
}

static void
bq_from_dbq(bpf_biquad *bq, double_biquad *dbq) {
	bq->a1 = dsp_from_double(dbq->a1);
	bq->a2 = dsp_from_double(dbq->a2);

	/* We do not need to store these three coefficients, as they always have
	 * the same value. */
	//bq->b0 = dsp_from_double(dbq->b0);
	//bq->b1 = dsp_from_double(dbq->b1);
	//bq->b2 = dsp_from_double(dbq->b2);
}

void
design_bpf(bpf_cascaded_biquad *cbq, double fc, double fw) {
	analog_layout analog = {0};
	digital_layout digital = {0};
	analog_design(&analog);

	double_biquad d_biquads[NUM_STAGES] = {0};

	band_pass_transform(&analog, &digital, fc, fw);

	for(int i = 0; i < NUM_STAGES; ++i) {
		bq_from_pzp(&d_biquads[i], &digital.poles[i]);
	}

	double response = sqrt(norm(cbq_response(cbq, d_biquads, digital.w / (2 * pi))));

	/* Scale will be applied separately. Do not apply the scale to the coefficients
	 * directly like IIR1 does. This ensures we get higher precision later. */
	cbq->scale = dsp_from_double(digital.gain / response);

	for(int i = 0; i < NUM_STAGES; ++i) {
		bq_from_dbq(&cbq->biquads[i], &d_biquads[i]);
	}
}