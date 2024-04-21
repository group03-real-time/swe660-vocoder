#include "audio_params.h"

#include "pru/pru_interface.h"
#include "gpio.h"

#define INPUT_MAX 65536 /* NOTE: Must be synchronized with ADC_VIRTUAL_SAMPLES */

/* Each param_ function performs some kind of transformation that maps the
 * samples coming from the ADC into values usable within the DSP code.
 *
 * These transformations may use the floating point functions for simplicty,
 * as they are not called very often and so do not have to be too fast. */

static dsp_num
param_exponential_lerp_factor(uint32_t adc_value) {
	return dsp_from_double(0.01 * pow(0.9999, adc_value));
}

static dsp_num
param_linear(uint32_t adc_value) {
	const dsp_num mul = dsp_one / INPUT_MAX;
	return adc_value * mul;
}

static dsp_num
param_gain(uint32_t adc_value) {
	return dsp_from_double(pow(0.9999, adc_value));
}

static int32_t multiplexer_idx = 0;

static gpio_pin multiplex_0 = GPIO_PIN_INVALID;
static gpio_pin multiplex_1 = GPIO_PIN_INVALID;
static gpio_pin multiplex_2 = GPIO_PIN_INVALID;
static gpio_pin multiplex_3 = GPIO_PIN_INVALID;

typedef struct {
	uintptr_t    offset;
	param_adc_fn fn;
} multiplex_seq_entry;

#define ENTRY(member, the_fn) {\
	.offset = offsetof(audio_params, member),\
	.fn = the_fn\
}

#define MULTIPLEX_PIN_0 67
#define MULTIPLEX_PIN_1 68
#define MULTIPLEX_PIN_2 44
#define MULTIPLEX_PIN_3 26

static multiplex_seq_entry
multiplex_sequencer[] = {
	ENTRY(output_gain, param_gain)
};

#define SEQUENCER_LEN (sizeof(multiplex_sequencer) / sizeof(*multiplex_sequencer))

static void
multiplex_gpio_write() {
	/* Write the next index to the GPIO pins.
	 * Write low bits first.
	 * 
	 * This is so that there's always a valid thing connected to the ADC.
	 * 
	 * In particular, going from 1101 -> 0, for example, is fine, because if
	 * we have 1101, we also have 1100, 1000, and 0000.
	 * 
	 * Similarly, going from 0111 to 1000 is fine, because we have 0110, 0100,
	 * and 0000.
	 * 
	 * If we went from high bits first, then going from 0111 to 1000 would also
	 * go through 1111, 1011, and 1001.
	 * 
	 * Of course, the ideal way to do this would probably be to choose some
	 * GPIO pins all on the same GPIO register, and then do the entire update
	 * in a single 4-byte write. But, oh well. */
	gpio_write(multiplex_0, !!(multiplexer_idx & 0x1));
	gpio_write(multiplex_1, !!(multiplexer_idx & 0x2));
	gpio_write(multiplex_2, !!(multiplexer_idx & 0x4));
	gpio_write(multiplex_3, !!(multiplexer_idx & 0x8));
}

void
audio_params_init_multiplexer() {
	multiplex_0 = gpio_open(MULTIPLEX_PIN_0, false);
	multiplex_1 = gpio_open(MULTIPLEX_PIN_1, false);
	multiplex_2 = gpio_open(MULTIPLEX_PIN_2, false);
	multiplex_3 = gpio_open(MULTIPLEX_PIN_3, false);
}

void
audio_params_tick_multiplexer(audio_params *out) {
	multiplex_seq_entry *seq = &multiplex_sequencer[multiplexer_idx];

	/* Figure out where the dsp_num we want to write to is inside the audio_params */
	dsp_num *out_ptr = (dsp_num*)((uintptr_t)out + seq->offset);

	dsp_num input = pru_adc_read_without_reset(1);
	*out_ptr = seq->fn(input); /* All sequencer values are on channel 1 */

	/* Finally, update the sequencer so that the next tick will udpate the next value */
	multiplexer_idx += 1;
	if(multiplexer_idx >= SEQUENCER_LEN) multiplexer_idx = 0;

	/* Update the multiplexer address for the next tick */
	multiplex_gpio_write();
	
	/* Notify the ADC to reset the sample after the multiplexer is updated.
	 * At this point, the data coming in on that channel *should* correspond
	 * to the new channel. */
	pru_adc_reset(1);
}

void
audio_params_default(audio_params *ap) {
	ap->attack  = dsp_one;
	ap->decay   = dsp_one;
	ap->sustain = dsp_one;
	ap->release = dsp_one;

	ap->noise_gain  = dsp_from_double(1 / 32.0);
	ap->output_gain = dsp_from_double(1 / 32.0);

	for(int i = 0; i < 3; ++i) {
		ap->oscs[i].gain     = dsp_one;
		ap->oscs[i].semitone = 0;
		ap->oscs[i].shape    = dsp_one;
	}
}