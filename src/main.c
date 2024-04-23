#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include <sched.h>

#include "app.h"
#include "gpio.h"

#include "audio_params.h"

#include "dsp/vocoder.h"
#include "dsp/synth.h"

#include "pru/pru_interface.h"

#include "buttons.h"

#define AUDIO_PARAM_TICK_RATE 183
#define BUTTON_READ_RATE (735 / (BUTTON_COUNT * BUTTON_DEBOUNCE))

int
main_app(int argc, char **argv, bool just_synth) {
	int delay_length = 0;

	if(argc >= 3) {
		delay_length = atoi(argv[2]);
		if(delay_length < 0) {
			app_fatal_error("must provide positive delay length (or none)");
		}
	}

	/* utsname */
	app_show_utsname();

	/* Use the loop for nicer exiting */
	app_init_loop();

	/* IMPORTANT:
	 * We were using the real time scheduler, but it turns out it introduces
	 * stuttering in the output signal for some reason.
	 * 
	 * Using the non-real time scheduler reuslts in a perfectly fine signal,
	 * as can be seen if running -synth. */

	vocoder voc;
	vc_init(&voc); /* vocoder */

	synth syn;
	synth_init(&syn); /* synth */

	audio_params params;
	audio_params_default(&params);

	/* We're using the multiplexer here, so start it up */
	audio_params_init_multiplexer();

	/* Also using the buttons */
	init_button_arr();

	/* These should be called as close as possible to when we start the loop */
	pru_audio_prepare_writing();
	pru_audio_prepare_reading();

	int audio_param_tick = 0;
	int button_tick_count = 0;
	int synth_debug_tick = 0;

	int button_inner_loop_count = 0;

	dsp_num *delay = NULL;
	int delay_write = delay_length - 1;
	int delay_read = 0;
	if(delay_length > 0) {
		delay = calloc(delay_length, sizeof(*delay));
		if(!delay) {
			app_fatal_error("could not allocate delay buffer");
		}
	}

	while(app_running) {
		/* Update button and audio params before the main DSP code to slightly
		 * reduce latency */
		button_tick_count += 1;
		if(button_tick_count >= BUTTON_READ_RATE) {
			button_tick_count = 0;
			button_tick(&syn, button_inner_loop_count, true);

			button_inner_loop_count += 1;
			if(button_inner_loop_count >= BUTTON_COUNT) {
				button_inner_loop_count = 0;
			}			
		}

		audio_param_tick += 1;
		if(audio_param_tick >= AUDIO_PARAM_TICK_RATE) {
			audio_param_tick = 0;
			audio_params_tick_multiplexer(&params, false);
		}

		synth_debug_tick += 1;
		if(synth_debug_tick >= 500) {
			synth_print_active_notes(&syn);
			synth_debug_tick = 0;
		}

		/* Read the modulator signal from the microphone */
		uint32_t modulator = pru_audio_read();

		/* Compute the carrier signal from the synthesizer */
		dsp_num carrier = synth_process(&syn, &params);

		/* The output signal is vocoded */
		dsp_num out = vc_process(&voc, modulator, carrier);

		if(just_synth) {
			/* Note: It's fine that we're wasting a bunch of time also
			 * computing the vocoder. This makes it easy to figure out if
			 * the vocoder is taking too long. (Just listen to the synth for
			 * jank). */
			out = carrier;
		}

		/* Apply output gain then write to the PRU */
		out = dsp_mul(out, params.output_gain);

		if(delay_length > 0) {
			delay[delay_write] = out;
			delay_write = (delay_write + 1) % delay_length;

			pru_audio_write(delay[delay_read]);
			delay_read = (delay_read + 1) % delay_length;
		}
		else {
			pru_audio_write(out);	
		}
	}

	return 0;
}