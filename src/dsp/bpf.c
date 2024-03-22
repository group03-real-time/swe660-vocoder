#include "bpf.h"

#include <string.h>
#include <math.h>
#include <complex.h>

typedef struct {
	double a1;
	double a2;
	double b0;
	double b1;
	double b2;
} double_biquad;

/* Output can be read out of eq->y[0] */
void 
biquad_update_even(biquad *bq, dsp_num *x, dsp_num *y) {
	/* Each biquad will update the corresponding y array. WE assume x was already
	 * updated. */
	//memmove(eq->x + 1, eq->x, sizeof(*eq->x) * 2);
	memmove(y + 1, y, sizeof(*y) * 2);

	/* Assume that x[0] is the new sample */

	/* Finally, compute y[0] */
	/* TODO: Note: b1 is zero for bpf, which is the only filter type we're using.
	 * So, just don't include it in the equation, for speed. */


	/* Optimization: According to some analysis, it appears that b0 and b2 are always one.
	 * So simply remove the multiplication. */
	//y[0] = /*bq->b0 **/ x[0] + bq->b1 * x[1] + /*bq->b2 * */x[2]
	//			         - bq->a1 * y[1] - bq->a2 * y[2];
	y[0] = x[0]
	     + (x[1] << 1) /* even index: b1 = 2 */
		 + x[2]
		 - dsp_mul(bq->a1, y[1])
		 - dsp_mul(bq->a2, y[2]);

#ifdef DSP_FLOAT
	/* Flush denormalized values for 11x speed improvement on x86 */
	if(dsp_abs(y[0]) < 1.175494350822287508e-38) {
		y[0] = 0.0;
	}
#endif
}

void 
biquad_update_odd(biquad *bq, dsp_num *x, dsp_num *y) {
	/* Each biquad will update the corresponding y array. WE assume x was already
	 * updated. */
	//memmove(eq->x + 1, eq->x, sizeof(*eq->x) * 2);
	memmove(y + 1, y, sizeof(*y) * 2);

	/* Assume that x[0] is the new sample */

	/* Finally, compute y[0] */
	/* TODO: Note: b1 is zero for bpf, which is the only filter type we're using.
	 * So, just don't include it in the equation, for speed. */


	/* Optimization: According to some analysis, it appears that b0 and b2 are always one.
	 * So simply remove the multiplication. */
	//y[0] = /*bq->b0 **/ x[0] + bq->b1 * x[1] + /*bq->b2 * */x[2]
	//			         - bq->a1 * y[1] - bq->a2 * y[2];
	y[0] = x[0]
	     - (x[1] << 1) /* odd index: b1 = -2 */
		 + x[2]
		 - dsp_mul(bq->a1, y[1])
		 - dsp_mul(bq->a2, y[2]);

#ifdef DSP_FLOAT
	/* Flush denormalized values for 11x speed improvement on x86 */
	if(dsp_abs(y[0]) < 1.175494350822287508e-38) {
		y[0] = 0.0;
	}
#endif
}

void 
biquad_update_scaled_even(biquad *bq, dsp_num *x, dsp_num *y, dsp_num scale) {
	/* Each biquad will update the corresponding y array. WE assume x was already
	 * updated. */
	//memmove(eq->x + 1, eq->x, sizeof(*eq->x) * 2);
	memmove(y + 1, y, sizeof(*y) * 2);

	/* Assume that x[0] is the new sample */

	/* Finally, compute y[0] */
	/* TODO: Note: b1 is zero for bpf, which is the only filter type we're using.
	 * So, just don't include it in the equation, for speed. */


	/* Optimization: According to some analysis, it appears that b0 and b2 are always one.
	 * So simply remove the multiplication. */
	y[0] = dsp_mul(x[0], scale)
	     + dsp_mul(x[1] << 1, scale) /* even index: b1 = 2 */
		 + dsp_mul(x[2], scale)
		 - dsp_mul(bq->a1, y[1])
		 - dsp_mul(bq->a2, y[2]);
	/*y[0] = dsp_mul(dsp_mul(bq->b0, x[0]), scale)
	     + dsp_mul(dsp_mul(bq->b1, x[1]), scale)
		 + dsp_mul(dsp_mul(bq->b2, x[2]), scale)
		 - dsp_mul(bq->a1, y[1])
		 - dsp_mul(bq->a2, y[2]);*/

#ifdef DSP_FLOAT
	/* Flush denormalized values for 11x speed improvement on x86 */
	if(dsp_abs(y[0]) < 1.175494350822287508e-38) {
		y[0] = 0.0;
	}
#endif
}

/* Assume x was already updated */
float
cbiquad_update(cascaded_biquad *bq, dsp_num *x) {
	biquad_update_scaled_even(&bq->biquads[0], x, bq->y_array[0], bq->scale);

	/* Note: NUM_STAGES must be at least 2 */
	biquad_update_odd(&bq->biquads[1],
			bq->y_array[0],
			bq->y_array[1]);

	for(int i = 2; i < NUM_STAGES; i += 2) {
		biquad_update_even(&bq->biquads[i],
			bq->y_array[i - 1],
			bq->y_array[i]);
		biquad_update_odd(&bq->biquads[i + 1],
			bq->y_array[i + 1 - 1],
			bq->y_array[i + 1]);
	}
	return bq->y_array[NUM_STAGES - 1][0];
}

typedef struct {
	complex p1;
	complex z1;
	complex p2;
	complex z2;
} pole_zero_pair;

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

#define POLE_ZERO_PAIR_CONJ(pole, zero)\
(pole_zero_pair){\
	.p1 = pole,\
	.z1 = zero,\
	.p2 = conj(pole),\
	.z2 = conj(zero)\
}

const double doublePi	=3.1415926535897932384626433832795028841971;
const double doublePi_2	=1.5707963267948966192313216916397514420986;
const double doubleLn2  =0.69314718055994530941723212145818;
const double doubleLn10	=2.3025850929940456840179914546844;

static inline
complex polar(double r, double theta) {
	return r * (cos(theta) + I * sin(theta));
}

static inline
complex addmul(complex a, double v, complex b) {
	 return (creal(a) + v * creal(b)) + I * (cimag(a) + v * cimag(b));
}

void analog_design(analog_layout *analog) {
	const double n2 = 2 * NUM_STAGES;
	const int pairs = NUM_STAGES / 2;
	for (int i = 0; i < pairs; ++i)
	{
		complex pole = polar(1., doublePi_2 + (2 * i + 1) * doublePi / n2);
		complex zero = INFINITY;

		analog->poles[i] = POLE_ZERO_PAIR_CONJ(pole, zero);
		//printf("poles[%d].p1, z1 = %f %f ; %f %f\n", i, creal(analog->poles[i].p1), cimag(analog->poles[i].p1), creal(analog->poles[i].z1), cimag(analog->poles[i].z1));
	}

	analog->w    = 0.0;
	analog->gain = 1.0;
}

typedef struct {
	complex first;
	complex second;
} complex_pair;

#define COMPLEX_PAIR(a, b) (complex_pair){.first = a, .second = b}

complex_pair bp_transform_pair(complex c, double b, double a2, double b2, double ab_2) {
	if (creal(c) == INFINITY)
		return COMPLEX_PAIR(-1, 1);
	
	c = (1. + c) / (1. - c); // bilinear
	//printf("bilinear c = %f, %f\n", creal(c), cimag(c));
	
	complex v = 0;
	v = addmul (v, 4 * (b2 * (a2 - 1) + 1), c);
	v += 8 * (b2 * (a2 - 1) - 1);
	v *= c;
	//printf("v @ times c = %f, %f\n", creal(v), cimag(v));
	v += 4 * (b2 * (a2 - 1) + 1);
	v = csqrt(v);
	//printf("v = %f, %f\n", creal(v), cimag(v));
	
	complex u = -v;
	u = addmul (u, ab_2, c);
	u += ab_2;
	
	v = addmul (v, ab_2, c);
	v += ab_2;
	
	complex d = 0;
	d = addmul (d, 2 * (b - 1), c) + 2 * (1 + b);
	
	return COMPLEX_PAIR (u/d, v/d);
}

void band_pass_transform(analog_layout *analog, digital_layout *digital, double fc, double fw) {
	//if (!(fc < 0.5)) throw_invalid_argument(cutoffError);
	//if (fc < 0.0) throw_invalid_argument(cutoffNeg);

	//digital.reset ();
	
	const double ww = 2 * doublePi * fw;
	
	// pre-calcs
	double wc2 = 2 * doublePi * fc - (ww / 2);
	double wc  = wc2 + ww;
	
	// what is this crap?
	if (wc2 < 1e-8)
		wc2 = 1e-8;
	if (wc  > doublePi-1e-8)
		wc  = doublePi-1e-8;
	
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

		//printf("p1.first = %f %f\n", creal(p1.first), cimag(p1.first));

		digital->poles[i * 2]     = POLE_ZERO_PAIR_CONJ(p1.first, z1.first);
		digital->poles[i * 2 + 1] = POLE_ZERO_PAIR_CONJ(p1.second, z1.second);
		//const PoleZeroPair& pair = analog[i];
		//ComplexPair p1 = transform (pair.poles.first);
		//ComplexPair z1 = transform (pair.zeros.first);
		
		//digital.addPoleZeroConjugatePairs (p1.first, z1.first);
		//digital.addPoleZeroConjugatePairs (p1.second, z1.second);
	}
	
	double wn = analog->w;
	digital->w = 2 * atan (sqrt (tan ((wc + wn)* 0.5) * tan((wc2 + wn)* 0.5)));
	digital->gain = analog->gain;
}

double norm(complex c) {
	double i = cimag(c);
	double r = creal(c);
	return i * i + r * r;
}

void bq_set_coefficients(double_biquad *bq, double a0, double a1, double a2, double b0, double b1, double b2) {
	bq->a1 = a1 / a0;
	bq->a2 = a2 / a0;
	bq->b0 = b0 / a0;
	bq->b1 = b1 / a0;
	bq->b2 = b2 / a0;

	//printf("coefficients [a1 a2 b0 b1 b2] = %f\t%f\t%f\t%f\t%f\n", bq->a1, bq->a2, bq->b0, bq->b1, bq->b2);
}

void bq_from_pzp(double_biquad *bq, pole_zero_pair *pzp) {
	const double a0 = 1;
		double a1;
		double a2;
		const char errMsgPole[] = "imaginary parts of both poles need to be 0 or complex conjugate";
		const char errMsgZero[] = "imaginary parts of both zeros need to be 0 or complex conjugate";

		if (cimag(pzp->p1) != 0)
		{
			//if (pole2 != std::conj (pole1))
			//	throw_invalid_argument(errMsgPole);
			a1 = -2 * creal(pzp->p1);
			a2 = norm(pzp->p1);
		}
		else
		{
			//if (pole2.imag() != 0)
			//	throw_invalid_argument(errMsgPole);
			a1 = -(creal(pzp->p1) + creal(pzp->p2));
			a2 =   creal(pzp->p1) * creal(pzp->p2);
		}

		const double b0 = 1;
		double b1;
		double b2;

		if (cimag(pzp->z1) != 0)
		{
			//if (zero2 != std::conj (zero1))
			//	throw_invalid_argument(errMsgZero);
			b1 = -2 * creal(pzp->z1);
			b2 = norm(pzp->z1);
		}
		else
		{
			//if (zero2.imag() != 0)
			//	throw_invalid_argument(errMsgZero);

			b1 = -(creal(pzp->z1) + creal(pzp->z2));
			b2 =   creal(pzp->z1) * creal(pzp->z2);
		}

		bq_set_coefficients(bq, a0, a1, a2, b0, b1, b2);
}



complex cbq_response(cascaded_biquad *cbq, double_biquad *dbqs, double normalized_frequency) {
	//if (normalized_frequency > 0.5) throw_invalid_argument(maxFError);
	//if (normalized_frequency < 0.0) throw_invalid_argument(minFError);
	double w = 2 * doublePi * normalized_frequency;
	const complex czn1 = polar(1., -w);
	const complex czn2 = polar(1., -2 * w);
	complex ch = 1.0;
	complex cbot = 1.0;

	//const Biquad* stage = m_stageArray;
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

void cbq_apply_scale(double_biquad *bq, double scale) {
	/* Apparently only applies to the first biquad */
	bq->b0 *= scale;
	bq->b1 *= scale;
	bq->b2 *= scale;
}

void bq_from_dbq(biquad *bq, double_biquad *dbq) {
	bq->a1 = dsp_from_double(dbq->a1);
	bq->a2 = dsp_from_double(dbq->a2);
	//bq->b0 = dsp_from_double(dbq->b0);
	//bq->b1 = dsp_from_double(dbq->b1);
	//bq->b2 = dsp_from_double(dbq->b2);

	//printf("coefficients [a1 a2 b0 b1 b2] = %f\t%f\t%f\t%f\t%f\n", dsp_to_float(bq->a1), dsp_to_float(bq->a2), dsp_to_float(bq->b0), dsp_to_float(bq->b1), dsp_to_float(bq->b2));
}

void design_bpf(cascaded_biquad *cbq, double fc, double fw) {
	analog_layout analog = {0};
	digital_layout digital = {0};
	analog_design(&analog);

	double_biquad d_biquads[NUM_STAGES] = {0};

	band_pass_transform(&analog, &digital, fc, fw);

	for(int i = 0; i < NUM_STAGES; ++i) {
		bq_from_pzp(&d_biquads[i], &digital.poles[i]);
	}

	double response = sqrt(norm(cbq_response(cbq, d_biquads, digital.w / (2 * doublePi))));
	//cbq_apply_scale(&d_biquads[0], digital.gain / response);

	/* Scale will be applied separately */
	cbq->scale = dsp_from_double(digital.gain / response);

	for(int i = 0; i < NUM_STAGES; ++i) {
		bq_from_dbq(&cbq->biquads[i], &d_biquads[i]);
	}
}