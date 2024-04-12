#include <stdint.h>
#include <pru_cfg.h>
#include <sys_tscAdcSs.h> /* TI ADC library */
#include "resource_table_empty.h"

#include "firmware.h"

volatile register unsigned int __R30;
volatile register unsigned int __R31;

#define sampler ((volatile struct pru0_ds*)(0x200))

#define STEPCONFIG(idx) ADC_TSC.STEPCONFIG ## idx
#define STEPCONFIG_bit(idx) ADC_TSC.STEPCONFIG ## idx ## _bit
#define STEPENABLE(idx) ADC_TSC.STEPENABLE_bit.STEP ## idx

#define STEPDELAY(idx) ADC_TSC.STEPDELAY ## idx
#define STEPDELAY_bit(idx) ADC_TSC.STEPDELAY ## idx ## _bit

#define STEPCONFIG_FOR_APP(idx, SEL_INP_SWC_3_0_val, FIFO_SELECT_val)\
do {\
	STEPCONFIG(idx) = 0; /* Reset everything to 0 -- most options are zero */\
	STEPCONFIG_bit(idx).FIFO_SELECT = FIFO_SELECT_val; /* Choose FIFO */\
	STEPCONFIG_bit(idx).SEL_RFM_SWC_1_0 = 3; /* VREF */\
	STEPCONFIG_bit(idx).SEL_RFP_SWC_2_0 = 3; /* ADCREF */\
	STEPCONFIG_bit(idx).SEL_INP_SWC_3_0 = SEL_INP_SWC_3_0_val; /* Which pin */\
	STEPCONFIG_bit(idx).SEL_INM_SWC_3_0 = 8;/* ADCREF */\
	STEPCONFIG_bit(idx).AVERAGING = 0; /* 16 samples */\
	STEPCONFIG_bit(idx).MODE = 1; /* SW-enabled, continuous */\
\
	STEPENABLE(idx) = 1;\
	STEPDELAY_bit(idx).OPENDELAY = 0x98; /* Standard value..? */\
} while(0)

static void config_adc() {
	//while (!(CM_WKUP_ADC_TSC_CLKCTRL == 0x02)) {
	//	CM_WKUP_CLKSTCTRL = 0;
	//	CM_WKUP_ADC_TSC_CLKCTRL = 0x02;
		/* Optional: implement timeout logic. */
	//}

	ADC_TSC.SYSCONFIG_bit.IDLEMODE = 1; /* Never idle..? */

	ADC_TSC.CTRL_bit.ENABLE = 0;
	ADC_TSC.CTRL_bit.STEPCONFIG_WRITEPROTECT_N_ACTIVE_LOW = 1;
	ADC_TSC.ADC_CLKDIV_bit.ADC_CLKDIV = 0; /* No clock divide */

	ADC_TSC.STEPENABLE = 0; /* disable all steps */

	/* Setup all steps */
	STEPCONFIG_FOR_APP(1, 0, 1);
	//STEPCONFIG_FOR_APP(2, 1, 0);
	STEPCONFIG_FOR_APP(3, 0, 1);
	//STEPCONFIG_FOR_APP(4, 2, 0);
	STEPCONFIG_FOR_APP(5, 0, 1);
	//STEPCONFIG_FOR_APP(6, 3, 0);
	STEPCONFIG_FOR_APP(7, 0, 1);
	//STEPCONFIG_FOR_APP(8, 4, 0);
	STEPCONFIG_FOR_APP(9, 0, 1);
	//STEPCONFIG_FOR_APP(10, 5, 0);
	STEPCONFIG_FOR_APP(11, 0, 1);
	STEPCONFIG_FOR_APP(12, 6, 0); /* Do not need val 7 */

	ADC_TSC.CTRL_bit.STEPCONFIG_WRITEPROTECT_N_ACTIVE_LOW = 0;
	/* Need to store the ID tag in the fifo so we know which value was written */
	ADC_TSC.CTRL_bit.STEP_ID_TAG = 1;
	ADC_TSC.CTRL_bit.ENABLE = 1;
}

void main(void) {

	/* For AIN0 (the microphone), the samples are averaged over time until
	 * the other PRU reads them. This ensures that we get a low-pass filtered
	 * signal that should be at least reasonably good. */
	uint32_t audio_sample_count = 0;
	uint32_t audio_sample_total = 0;

	sampler->magic = PRU0_MAGIC_NUMBER;

	int i;
	for(i = 0; i < 8; ++i) {
		sampler->samples[i] = 0;
	}
	sampler->audio_sample_avg = 0;
	
	/* Clear SYSCFG[STANDBY_INIT] to enable OCP master port */
	CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;

	config_adc();

	for(;;) {
		if(ADC_TSC.FIFO0COUNT > 0) {
			uint32_t next = ADC_TSC.FIFO0DATA;
			uint32_t chan = (next >> 16) & 0xF;
			uint32_t val = next & 0xFFF;
			sampler->samples[chan] = val;
		}

		if(ADC_TSC.FIFO1COUNT > 0) {
			/* Don't need the channel as this is always channel 0. */
			uint32_t sample = ADC_TSC.FIFO1DATA_bit.ADCDATA;

			if(sampler->audio_sample_reset) {
				sampler->last_audio_sample_count = audio_sample_count;

				audio_sample_count = 0;
				audio_sample_total = 0;
				sampler->audio_sample_reset = 0;
			}

			audio_sample_total += sample;
			audio_sample_count += 1;

			sampler->audio_sample_avg = (audio_sample_total * AUDIO_VIRTUAL_SAMPLECOUNT) / audio_sample_count;
		}
	}

	
	__halt();
}

// Turns off triggers
#pragma DATA_SECTION(init_pins, ".init_pins")
#pragma RETAIN(init_pins)
const char init_pins[] =  
	"/sys/class/leds/beaglebone:green:usr3/trigger\0none\0" \
	"\0\0";
