#include <stdio.h>
#include <assert.h>

#include "app.h"
#include "gpio.h"

#include "audio_params.h"

#include "dsp/vocoder.h"
#include "dsp/synth.h"

#include "pru/pru_interface.h"

#include "buttons.h"

#define AUDIO_PARAM_TICK_RATE 46
#define BUTTON_READ_RATE (735 / BUTTON_DEBOUNCE)

int
main_app(int argc, char **argv) {
	/* utsname */
	app_show_utsname();

	/* Use the loop for nicer exiting */
	app_init_loop();

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

	while(app_running) {
		/* Update button and audio params before the main DSP code to slightly
		 * reduce latency */
		button_tick_count += 1;
		if(button_tick_count >= BUTTON_READ_RATE) {
			button_tick_count = 0;
			button_tick(&syn, true);			
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

		/* Apply output gain then write to the PRU */
		out = dsp_mul(out, params.output_gain);
		pru_audio_write(out);	
	}

	return 0;
}