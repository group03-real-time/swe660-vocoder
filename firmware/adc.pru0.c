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

	/* Step configuration:
	 * First, we want to sample the audio channel as often as possible. What 
	 * this means in practice is simply that we sample the other channels
	 * as rarely as possible.
	 * 
	 * Well, not quite. For simplicity, we want to use a single fixed sequencer
	 * setup--just walk through the same sampling steps over and over.
	 * 
	 * So what we do is use 7/8 steps for sampling the audio data, and the 8th
	 * step for sampling one of the other channels. Because there are only 16
	 * steps, this setup supports 2 of the channels (which, with 2 multiplexers,
	 * supports a total of 32 possible knobs). */
	STEPCONFIG_FOR_APP(1, 0, 1);
	STEPCONFIG_FOR_APP(2, 0, 1);
	STEPCONFIG_FOR_APP(3, 0, 1);
	STEPCONFIG_FOR_APP(4, 0, 1);
	STEPCONFIG_FOR_APP(5, 0, 1);
	STEPCONFIG_FOR_APP(6, 0, 1);
	STEPCONFIG_FOR_APP(7, 0, 1);
	STEPCONFIG_FOR_APP(8, 1, 0); /* Sample for ADC channel 1 */

	//STEPCONFIG_FOR_APP(9, 0, 1);
	//STEPCONFIG_FOR_APP(10, 0, 1);
	//STEPCONFIG_FOR_APP(11, 0, 1);
	//STEPCONFIG_FOR_APP(12, 0, 1);
	//STEPCONFIG_FOR_APP(13, 0, 1);
	//STEPCONFIG_FOR_APP(14, 0, 1);
	//STEPCONFIG_FOR_APP(15, 0, 1);
	//STEPCONFIG_FOR_APP(16, 2, 0); /* Sample for ADC channel 2 */

	ADC_TSC.CTRL_bit.STEPCONFIG_WRITEPROTECT_N_ACTIVE_LOW = 0;
	/* Need to store the ID tag in the fifo so we know which value was written */
	ADC_TSC.CTRL_bit.STEP_ID_TAG = 1;
	ADC_TSC.CTRL_bit.ENABLE = 1;
}

void main(void) {

	/* For each channel, the samples are averaged over time until something
	 * reads them. For the audio channel, this is until the other PRU reads them.
	 * 
	 * This provides a few functionalites.
	 * 
	 * First, it simply gives a less noisy reading. The ADC is likely to read
	 * slightly different values over time a lot of the time.
	 * 
	 * Second, it low passes filters the signal. This is most important for
	 * the audio signal, but essentially amounts to smoothing for all of the
	 * signals. This low pass filter is not likely to be very high quality,
	 * but it's better than nothing.
	 * 
	 * Finally, it provides more bit depth. In essence, we're doing the following:
	 * 
	 * average = samples / sample_count
	 * 
	 * But, because the division loses information, we rearrange like so:
	 * 
	 * F * average = F * samples / sample_count = (F * samples) / sample_count
	 * 
	 * And then provide this value F * average. F is one of either AUDIO_VIRTUAL_SAMPLECOUNT
	 * or ADC_VIRTUAL_SAMPLECOUNT.
	 * 
	 * As long as F >= sample_count, this prevents the loss of information. */

	sampler->magic = PRU0_MAGIC_NUMBER;

	int i;
	for(i = 0; i < 8; ++i) {
		sampler->samples[i] = 0;
		sampler->sample_count[i] = 0;
		sampler->sample_total[i] = 0;
		sampler->sample_reset[i] = 0;
	}
	
	/* Clear SYSCFG[STANDBY_INIT] to enable OCP master port */
	CT_CFG.SYSCFG_bit.STANDBY_INIT = 0;

	config_adc();

/* We use the same UPDATE_SAMPLE macro for both FIFOs -- FIFO 1 has all
 * the audio (channel 0) samples, while FIFO 0 has all the other samples.
 *
 * The process is as follows.
 * 
 * If the reset flag is set for the channel, then the total and count values
 * are reset to 0.
 * 
 * Otherwise, the total is increased by the sample, and the count is increased
 * by 1. This is because the average = total / count, as might be expected.
 * 
 * Finally, the average is simply computed every single time. This is because
 * the ADC only supports up to 200,000 samples/sec, while the PRU runs much
 * much faster. As such, it doesn't matter that we recompute the average each
 * time, and this ensures that the CPU side can simply read the average value.
 * 
 * That is, we don't want the CPU trying to read both the total and the sample
 * count, as one of those might change while it's reading the other. Instead,
 * the average value will always represent *some* kind of valid sample, so
 * the CPU can just read it directly. */
#define UPDATE_SAMPLE(chan, val, SCALING)\
do {\
	if(sampler->sample_reset[chan]) {\
		sampler->sample_count[chan] = 0;\
		sampler->sample_total[chan] = 0;\
		\
		sampler->sample_reset[chan] = 0;\
	}\
	\
	sampler->sample_total[chan] += val;\
	sampler->sample_count[chan] += 1;\
	\
	sampler->samples[chan] = (sampler->sample_total[chan]  * SCALING) / sampler->sample_count[chan];\
} while(0)

	for(;;) {
		if(ADC_TSC.FIFO0COUNT > 0) {
			uint32_t next = ADC_TSC.FIFO0DATA;
			uint32_t chan = (next >> 16) & 0xF;
			uint32_t val = next & 0xFFF;
			
			UPDATE_SAMPLE(1, val, ADC_VIRTUAL_SAMPLECOUNT);
		}

		if(ADC_TSC.FIFO1COUNT > 0) {
			/* Don't need the channel as this is always channel 0. */
			uint32_t sample = ADC_TSC.FIFO1DATA_bit.ADCDATA;

			UPDATE_SAMPLE(0, sample, AUDIO_VIRTUAL_SAMPLECOUNT);
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
