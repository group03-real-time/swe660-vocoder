#include "bpf.h"

#include <string.h>
#include <math.h>

/* Output can be read out of eq->y[0] */
void 
eq_update(rbj_eq *eq, float sample) {
	/* First, update arrays */
	memmove(eq->x + 1, eq->x, sizeof(*eq->x) * 2);
	memmove(eq->y + 1, eq->y, sizeof(*eq->y) * 2);

	/* Then, add new sample */
	eq->x[0] = sample;

	/* Finally, compute y[0] */
	/* TODO: Note: b1 is zero for bpf, which is the only filter type we're using.
	 * So, just don't include it in the equation, for speed. */
	eq->y[0] = eq->b0 * eq->x[0] + eq->b1 * eq->x[1] + eq->b2 * eq->x[2]
	                             - eq->a1 * eq->y[1] - eq->a2 * eq->y[2];
}

void
eq_create_bpf(rbj_eq *eq, double freq, double q) {
	double omega0 = 2 * 3.1415926535897932384 * freq;
	double sin_omega0 = sin(omega0);

	//double q = 0.707;
	//double q = 8.0;
	double alpha = sin_omega0 / (2.0 * q);

	//double alpha = sin_omega0 * sinh((log10(2) / 2) * bandwidth * sin_omega0);

	double b0 = alpha;
	double b1 = 0;
	double b2 = -alpha;
	double a0 = (1.0 + alpha);
	double a1 = -2.0 * cos(omega0);
	double a2 = (1.0 - alpha);

	/*double b0 = sin_omega0 / 2.0;
	double b1 = 0.0;
	double b2 = -sin_omega0 / 2.0;
	double a0 = 1.0 + alpha;
	double a1 = -2.0 * cos(omega0);
	double a2 = 1.0 - alpha;*/

	/* Normalize coefficients */
	b0 /= a0;
	b1 /= a0;
	b2 /= a0;
	a1 /= a0;
	a2 /= a0;

	eq->a1 = (float)a1;
	eq->a2 = (float)a2;
	eq->b0 = (float)b0;
	eq->b1 = (float)b1;
	eq->b2 = (float)b2;
}