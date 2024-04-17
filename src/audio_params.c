#include "audio_params.h"

#include "pru/pru_interface.h"
#include "gpio.h"

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
	ENTRY(attack, param_exponential_lerp_factor)
};

#define SEQUENCER_LEN (sizeof(multiplex_sequencer) / sizeof(*multiplex_sequencer))

static void
multiplex_gpio_write() {
	/* Write the next index to the GPIO pins */
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

	*out_ptr = seq->fn(pru_adc_read(1)); /* All sequencer values are on channel 1 */

	/* Finally, update the sequencer so that the next tick will udpate the next value */
	multiplexer_idx += 1;
	if(multiplexer_idx >= SEQUENCER_LEN) multiplexer_idx = 0;

	/* Update the multiplexer address for the next tick */
	multiplex_gpio_write();
}