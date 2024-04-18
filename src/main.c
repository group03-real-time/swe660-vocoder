#include <stdio.h>
#include <assert.h>

#include "app.h"
#include "gpio.h"
#include "good_user_input.h"

#include "audio_params.h"

#include "dsp/vocoder.h"
#include "dsp/synth.h"

#include "pru/pru_interface.h"

#define AUDIO_PARAM_TICK_RATE 46

int
main_app(int argc, char **argv) {
	/* utsname */
	app_show_utsname();

	app_init_loop();

	vocoder voc;
	vc_init(&voc);

	synth syn;
	synth_init(&syn);

	audio_params params;
	audio_params_default(&params);
	audio_params_init_multiplexer();

	/* Play some notes */
	synth_press(&syn, 0);
	synth_press(&syn, 7);
	synth_press(&syn, 12);
	synth_press(&syn, 28);

	pru_audio_prepare_writing();
	pru_audio_prepare_reading();

	int audio_param_tick = 0;

	while(app_running) {
		/* Read the modulator */
		uint32_t modulator = pru_audio_read();

		dsp_num carrier = synth_process(&syn, &params);

		dsp_num out = vc_process(&voc, modulator, carrier);

		/* Apply output gain */
		out = dsp_mul(out, params.output_gain);

		pru_audio_write(out);

		audio_param_tick += 1;
		if(audio_param_tick >= AUDIO_PARAM_TICK_RATE) {
			audio_param_tick = 0;
			audio_params_tick_multiplexer(&params);
		}
	}

	return 0;
}