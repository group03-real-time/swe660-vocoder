/**
 * A test file for testing that the DSP code works offline.
 * Uses the drwav.h library for reading and writing wav files.
*/

#include "vocoder.h"

#include <stdint.h>
#include <stdlib.h>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

static void
write_output(const char *fp, float *arr, int out_channels, int out_frames, int out_sr) {
	/* Writing the output data */
	drwav wav;
	
	// Setup the data format.
	drwav_data_format format;
    format.container     = drwav_container_riff;      // riff = normal data format.
    format.format        = DR_WAVE_FORMAT_IEEE_FLOAT; // FLOAT for 32 bit float data format. TODO: Consider if we want to write 16 bit wavs?
    format.channels      = out_channels;
    format.sampleRate    = out_sr;
    format.bitsPerSample = 32; // 32 bit float

	/* Write output */
	if(!drwav_init_file_write_sequential_pcm_frames(
		&wav,
		fp,
		&format,
		out_frames,
		NULL)) {
			printf("couldn't open output file %s\n", fp);
			exit(1);
		}

	drwav_write_pcm_frames(&wav, out_frames, arr);
}

int main(int argc, char **argv) {
	if(argc < 4) {
		printf("usage: %s <modulator.wav (voice)> <carrier.wav (synth)> <output.wav>\n", argv[0]);
		return 1;
	}

	const char *mod_fp = argv[1];
	const char *car_fp = argv[2];
	const char *out_fp = argv[3];

	printf("processing with\n\tmodulator = %s\n\tcarrier = %s\n\toutput = %s\n", mod_fp, car_fp, out_fp);

	unsigned int mod_channels;
	unsigned int mod_sr;
	drwav_uint64 mod_frames;

	unsigned int car_channels;
	unsigned int car_sr;
	drwav_uint64 car_frames;

	float *modulator = drwav_open_file_and_read_pcm_frames_f32(
		mod_fp,
		&mod_channels,
		&mod_sr,
		&mod_frames,
		NULL);

	if(!modulator) {
		printf("couldn't open modulator %s\n", mod_fp);
		return 2;
	}

	float *carrier = drwav_open_file_and_read_pcm_frames_f32(
		car_fp,
		&car_channels,
		&car_sr,
		&car_frames,
		NULL);

	if(!carrier) {
		printf("couldn't open carrier %s\n", car_fp);
		return 3;
	}

	if(mod_sr != SAMPLE_RATE) {
		printf("WARNING: modulator sample rate does not match output (%d vs %d)\n", mod_sr, SAMPLE_RATE);
	}
	if(car_sr != SAMPLE_RATE) {
		printf("WARNING: carrier sample rate does not match output (%d vs %d)\n", car_sr, SAMPLE_RATE);
	}

	unsigned int out_channels = 1;
	drwav_uint64 out_frames = (mod_frames > car_frames) ? mod_frames : car_frames;
	unsigned int out_sr = SAMPLE_RATE;

#define ARR calloc(out_frames * out_channels, sizeof(*out))
	float *out = calloc(out_frames * out_channels, sizeof(*out));

	float *mbpf_test[VOCODER_BANDS];
	float *e_test[VOCODER_BANDS];
	float *cbpf_test[VOCODER_BANDS];
	for(int i = 0; i < VOCODER_BANDS; ++i) {
		mbpf_test[i] = ARR;
		e_test[i] = ARR;
		cbpf_test[i] = ARR;
	}

	/* Initialize the vocoder */
	//vocoder voc;
	vocoder *voc = vc_new();

	int mi = 0;
	int ci = 0;

	for(int i = 0; i < out_frames; ++i) {
		float m = (mi < mod_frames) ? modulator[mi] : 0.0;
		float c = (ci < car_frames) ? carrier[ci] : 0.0;

		float o = dsp_to_float(vc_process(voc, m, c));
		out[i] = o;

		for(int j = 0; j < VOCODER_BANDS; ++j) {
			//mbpf_test[j][i] = voc.mod_filters[j].y[0];
			//cbpf_test[j][i] = voc.carrier_filters[j].y[0];
			//e_test[j][i] = voc->envelope_follow[j];
		}

		//if(i % 10000 == 0) {
			//printf("processed %d frames\n", i);
		//}

		/* Only use the leftmost channel */
		mi += mod_channels;
		ci += car_channels;
	}

#define WRITE(arr, fp) write_output(fp, arr, out_channels, out_frames, out_sr)

	WRITE(out, out_fp);

	for(int i = 0; i < VOCODER_BANDS; ++i) {
		char buf[64] = {0};
		//sprintf(buf, "mbpf_%d.wav", i);
		//WRITE(mbpf_test[i], buf);
		//printf(buf, "env_%d.wav", i);
		//WRITE(e_test[i], buf);
		//sprintf(buf, "cbpf_%d.wav", i);
		//WRITE(cbpf_test[i], buf);
	}
}