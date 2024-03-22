#ifndef WAV_H
#define WAV_H

#include "../dsp/dsp.h"

#include "../types.h"

typedef struct {
	dsp_num *buffer;
	uint64_t frames;
	uint16_t channels;
	uint32_t sample_rate;
} wav_io;

void wav_read_or_die(wav_io *io, const char *path);
void wav_blank_or_die(wav_io *io, uint64_t frames, uint32_t channels, uint32_t sample_rate);
void wav_write_or_warn(wav_io *io, const char *path);

#endif