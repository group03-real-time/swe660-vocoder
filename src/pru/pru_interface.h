#ifndef PRU_INTERFACE_H
#define PRU_INTERFACE_H

#include "types.h"

/**
 * Prepares the PRU input buffer for audio reading. Should be called before
 * calling pru_audio_read() in a tight loop.
 */
void pru_audio_prepare_reading();

/**
 * Prepares the PRU output buffer for audio writing. Should be called before
 * calling pru_audio_write() in a tight loop.
 */
void pru_audio_prepare_writing();

/**
 * Reads a single sample from the PRU audio system. Blocks if none are available.
 * The returned sample is in the range (-dsp_one / 2) to (dsp_one / 2), essentially.
*/
int32_t pru_audio_read();

/**
 * Writes a single sample to the PRU audio output system. Blocks if the output
 * buffer is full. The sample should be in the range -dsp_one to dsp_one.
 */
void pru_audio_write(int32_t sample);

/**
 * Reads a single averaged sample from the given ADC channel, but without resetting
 * the running average value. This ensures that the resetting step can be done
 * with more precise timing.
 */
int32_t pru_adc_read_without_reset(int32_t channel);

/**
 * Resets the running average for the given ADC channel. Should generally be called
 * right after reading that channel, so that the channel can start accumulating
 * data for the time of the next read.
 */
void pru_adc_reset(int32_t channel);

/**
 * Initializes the PRU subsystem and installs all the PRU firmware onto the 
 * BBB. This is necessary for anything else to work.
 */
void pru_init();

/**
 * Shuts down the PRU subsystem and stops the PRUs.
 */
void pru_shutdown();

#endif