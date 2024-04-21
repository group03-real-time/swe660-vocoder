#include "wav.h"

#include <stdlib.h>

#include "../app.h"

/* We use drwav for all low level WAV reading/writing logic.
 *
 * This library is a "header-only" library, so we simply commit the header file
 * into our code base and then include it here with this #define to get the
 * code implementation. */
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

void
wav_read_or_die(wav_io *io, const char *path) {
	if(!io) return;

	unsigned int channels;
	unsigned int sr;
	drwav_uint64 frames;

	float *samples = drwav_open_file_and_read_pcm_frames_f32(
		path,
		&channels,
		&sr,
		&frames,
		NULL);

	if(!samples) {
		app_fatal_error("could not open wav file for reading");
	}

	/* Convert the input samples into dsp_num */
	dsp_num *buffer = calloc(channels * frames, sizeof(*buffer));

	if(!buffer) {
		app_fatal_error("could not allocate wav file samples buffer");
	}

	uint64_t len = (frames * channels);
	for(uint64_t i = 0; i < len; ++i) {
		buffer[i] = dsp_from_double(samples[i]);
	}

	drwav_free(samples, NULL);

	io->buffer = buffer;
	io->frames = frames;
	io->channels = channels;
	io->sample_rate = sr;

	io->buffer_length = frames * channels;
}

void
wav_blank_or_die(wav_io *io, uint64_t frames, uint32_t channels, uint32_t sample_rate) {
	if(!io) return;

	dsp_num *buffer = calloc(channels * frames, sizeof(*buffer));
	if(!buffer) {
		app_fatal_error("could not allocate blank wav file samples buffer");
	}

	io->buffer = buffer;
	io->frames = frames;
	io->channels = channels;
	io->sample_rate = sample_rate;
	io->buffer_length = io->frames * io->channels;
}

void
wav_write_or_warn(wav_io *io, const char *path) {
	if(!io) return;

	/* Writing the output data */
	drwav wav;
	
	/* Setup the data format. */
	drwav_data_format format;
    format.container     = drwav_container_riff;      // riff = normal data format.
    format.format        = DR_WAVE_FORMAT_IEEE_FLOAT; // FLOAT for 32 bit float data format. TODO: Consider if we want to write 16 bit wavs?
    format.channels      = io->channels;
    format.sampleRate    = io->sample_rate;
    format.bitsPerSample = 32; // 32 bit float

	float *output_buffer = calloc(io->channels * io->frames, sizeof(*output_buffer));
	if(!output_buffer) {
		printf("WARNING: could allocate space for writing output file %s\n", path);
		return; /* And warn*/
	}

	uint64_t len = io->channels * io->frames;
	for(uint64_t i = 0; i < len; ++i) {
		output_buffer[i] = dsp_to_float(io->buffer[i]);
	}

	/* Write output */
	if(!drwav_init_file_write_sequential_pcm_frames(
		&wav,
		path,
		&format,
		io->frames,
		NULL)
	) {
		printf("WARNING: could not open output file %s\n", path);
		free(output_buffer);
		return;
	}

	drwav_write_pcm_frames(&wav, io->frames, output_buffer);

	drwav_uninit(&wav);

	free(output_buffer);
}