#ifndef PRU_INTERFACE_H
#define PRU_INTERFACE_H

#include "types.h"

void pru_audio_prepare_reading();
void pru_audio_prepare_writing();
int32_t pru_audio_read();
void pru_audio_write(int32_t sample);

int32_t pru_adc_read_without_reset(int32_t channel);
void pru_adc_reset(int32_t channel);

void pru_init();
void pru_shutdown();

#endif