#ifndef VOCODER_H
#define VOCODER_H

#include "dsp.h"
#include "bpf.h"

/* The main type used to process our samples. Takes two inputs: the modulator
 * (i.e. the voice signal) and the carrier (i.e. the synthesizer). */
/*typedef struct {
	rbj_eq mod_filters[VOCODER_BANDS];
	rbj_eq carrier_filters[VOCODER_BANDS];
	float envelope_follow[VOCODER_BANDS];
} vocoder;*/

#ifdef __cplusplus

extern "C" {

#endif

typedef struct vocoder vocoder;

vocoder *vc_new();
float vc_process(vocoder *v, float modulator, float carrier);

#ifdef __cplusplus

}

#endif

#endif