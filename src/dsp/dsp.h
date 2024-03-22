#ifndef DSP_H
#define DSP_H

#ifndef SAMPLE_RATE
	#define SAMPLE_RATE 44100
#endif
#define VOCODER_BANDS 8

#ifdef DSP_FLOAT

#include <math.h>

typedef float dsp_num;

/*static inline
dsp_num dsp_add(dsp_num a, dsp_num b) {
	return a + b;
}*/

static inline
dsp_num dsp_mul(dsp_num a, dsp_num b) {
	return a * b;
}

static inline
dsp_num dsp_div(dsp_num a, dsp_num b) {
	return a / b;
}

static inline
float dsp_to_float(dsp_num f) {
	return f;
}

static inline
dsp_num dsp_from_double(double d) {
	return (dsp_num)d;
}

static inline dsp_num
dsp_abs(dsp_num f) {
	return fabsf(f);
}

#define dsp_zero 0.0f
#define INPUT_EXTRA_MUL 1

#else

#include <stdint.h>

//#ifdef DSP_FIXED_32

#define DSP_POINT_IDX 29 /* only a handful of bits above 1.0 */
typedef int32_t dsp_num;
typedef int64_t dsp_largenum;

#define LARGER_T int64_t
#define INPUT_EXTRA_MUL 1
//#define INPUT_EXTRA_MUL 4

/*#else

#define DSP_POINT_IDX 59
typedef int64_t dsp_num;

#define LARGER_T __int128_t
#define INPUT_EXTRA_MUL 16

#endif*/

/* For now: no addition or subtraction, as the built in operators will work. */
/*static inline dsp_num
dsp_add(dsp_num a, dsp_num b) {
	return a + b;
}*/

static inline dsp_num
dsp_mul(dsp_num a, dsp_num b) {
	const LARGER_T a64 = a;
	const LARGER_T b64 = b;
	const LARGER_T res = a64 * b64;
	return (dsp_num)(res >> DSP_POINT_IDX);
}

static inline dsp_largenum
dsp_mul_large(dsp_num a, dsp_num b) {
	const LARGER_T a64 = a;
	const LARGER_T b64 = b;
	const LARGER_T res = a64 * b64;
	return res;
}

static inline dsp_num
dsp_compact(dsp_largenum num) {
	return (dsp_num)(num >> DSP_POINT_IDX);
}

static inline dsp_num
dsp_div(dsp_num a, dsp_num b) {
	const LARGER_T a64 = ((LARGER_T)a << DSP_POINT_IDX);
	const LARGER_T b64 = b;
	const LARGER_T res = a64 / b64;
	return (dsp_num)res;
}

static inline float
dsp_to_float(dsp_num f) {
	double b = (double)f;
	double shift = (double)(1LL << DSP_POINT_IDX);
	b /= shift;
	return (float)b;
}

static inline dsp_num 
dsp_from_double(double d) {
	double shift = (double)(1LL << DSP_POINT_IDX);
	d *= shift;
	return (dsp_num)d;
}

static inline dsp_num
dsp_abs(dsp_num f) {
	/* note: does not handle absolute minimum value */
	return (f < 0) ? -f : f;
}

#define dsp_zero 0



#endif



#endif