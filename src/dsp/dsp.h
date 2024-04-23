#ifndef DSP_H
#define DSP_H

#include <stdint.h>

/** 
 * NOTE: DSP_FLOAT IS NOT LONGER SUPPORTED.
 * The vocoder and dsp code still works with DSP_FLOAT, but the rest of the code
 * base does not. As such, do not use it.
 */
#ifdef DSP_FLOAT

#include <math.h>

typedef float dsp_num;
typedef float dsp_largenum;

/*static inline
dsp_num dsp_add(dsp_num a, dsp_num b) {
	return a + b;
}*/

static inline
dsp_num dsp_mul(dsp_num a, dsp_num b) {
	return a * b;
}

static inline dsp_largenum
dsp_mul_large(dsp_num a, dsp_num b) {
	return a * b;
}

static inline dsp_num
dsp_compact(dsp_largenum num) {
	return num;
}

static inline
dsp_num dsp_div(dsp_num a, dsp_num b) {
	return a / b;
}

static inline dsp_num
dsp_div_int_num(int32_t a, dsp_num b) {
	return a / b;
}

static inline dsp_num
dsp_div_int_denom(dsp_num a, int32_t b) {
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

static inline dsp_num
dsp_lshift(dsp_num expr, int shift) {
	int mask = (1 << shift);
	return expr * mask;
}

static inline dsp_num
dsp_rshift(dsp_num expr, int shift) {
	int mask = (1 << shift);
	return expr / mask;
}

#define dsp_zero 0.0f
#define dsp_one  1.0f
#define INPUT_EXTRA_MUL 1

#else

//#ifdef DSP_FIXED_32

#define DSP_POINT_IDX 29 /* only a handful of bits above 1.0 */
typedef int32_t dsp_num;
typedef int64_t dsp_largenum;

#define dsp_one ((dsp_num)0x20000000)

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

static inline dsp_num
dsp_div_int_num(int32_t a, dsp_num b) {
	const LARGER_T a64 = ((LARGER_T)a << (DSP_POINT_IDX + DSP_POINT_IDX));
	const LARGER_T b64 = b;
	const LARGER_T res = a64 / b64;
	return (dsp_num)res;
}

static inline dsp_num
dsp_div_int_denom(dsp_num a, int32_t b) {
	/* If a / b < 0, we need to make a bigger first, then shift back */
	const LARGER_T a64 = ((LARGER_T)a << DSP_POINT_IDX);
	const LARGER_T b64 = b;
	const LARGER_T res = (a64 / b64);
	return (dsp_num)(res >> DSP_POINT_IDX);
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

#define dsp_lshift(expr, shift) ((expr) << (shift))
#define dsp_rshift(expr, shift) ((expr) >> (shift))

#define dsp_zero 0



#endif



#endif