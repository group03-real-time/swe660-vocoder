#ifndef VOCODER_H
#define VOCODER_H

#include "dsp.h"
#include "bpf.h"

typedef struct {
	cascaded_biquad mod_filters[VOCODER_BANDS];
	cascaded_biquad car_filters[VOCODER_BANDS];
	dsp_num envelope_follow[VOCODER_BANDS];

	dsp_num mod_x[3];
	dsp_num car_x[3];

	dsp_num mod_ef;
	dsp_num sum_ef;
} vocoder;

void vc_init(vocoder *v);
dsp_num vc_process(vocoder *v, dsp_num modulator, dsp_num carrier);

#endif