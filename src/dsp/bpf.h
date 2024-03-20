#ifndef BPF_H
#define BPF_H

/* Struct for representing an EQ filter based on the RBJ cookbook. This source
 * can be found in different places, e.g. https://www.w3.org/TR/audio-eq-cookbook/
 * Obviously not the best kind of filters ever, but should work for our purposes.
 *
 * Represents a biquad filter in general. */
typedef struct {
	float x[3];
	float y[3];

	/* All coefficients normalized by a0 */
	float b0;
	float b1;
	float b2;

	float a1;
	float a2;
} rbj_eq;

/* Output can be read out of eq->y[0] */
void eq_update(rbj_eq *eq, float sample);

/* Note: frequency should be in interval [0, 0.5) */
void eq_create_bpf(rbj_eq *eq, double freq, double bandwidth);

#endif