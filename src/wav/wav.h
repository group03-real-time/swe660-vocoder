#ifndef WAV_H
#define WAV_H

#include "types.h"
#include "dsp/dsp.h"

/**
 * wav.h -- provides a simple interface to read and write wav files.
 * 
 * Does not provide for detailed error handling or the like, as this is meant
 * mostly for quickly running tests on the DSP code and similar. */


/* The structure used for reading/writing wavs */
typedef struct {
	dsp_num *buffer;
	uint64_t frames;
	uint16_t channels;
	uint32_t sample_rate;

	uint64_t buffer_length;
} wav_io;

/**
 * Either reads the given WAV file into the given struct, or exits the program
 * with a fatal error.
 */
void wav_read_or_die(wav_io *io, const char *path);

/**
 * Initializes a new WAV structure with all 0's, so it can be written with data.
 * If memory cannot be allocated, exits with a fatal error.
 */
void wav_blank_or_die(wav_io *io, uint64_t frames, uint32_t channels, uint32_t sample_rate);

/**
 * Either writes the given WAV data to the given path, or prints a warning to
 * the terminal saying it cannot be written.
 */
void wav_write_or_warn(wav_io *io, const char *path);

#endif