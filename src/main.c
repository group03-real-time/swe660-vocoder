#include <stdio.h>
#include <assert.h>

#include "app.h"
#include "gpio.h"
#include "good_user_input.h"

#include "dsp/vocoder.h"
#include "dsp/synth.h"

#include "pru/pru_interface.h"

int
main_app(int argc, char **argv) {
	/* utsname */
	app_show_utsname();

	app_init_loop();

	vocoder voc;
	vc_init(&voc);

	synth syn;
	synth_init(&syn);

	/* Play some notes */
	synth_press(&syn, 0);
	synth_press(&syn, 7);
	synth_press(&syn, 12);
	synth_press(&syn, 28);

	pru_audio_prepare_writing();
	pru_audio_prepare_reading();

	//int i = 0;

	while(app_running) {
		uint32_t modulator = pru_audio_read() * 1024;

		dsp_num carrier = synth_process(&syn);

		int32_t out = vc_process(&voc, modulator, carrier);

		//if(i == 200) { i = 0; printf("got data %d => %d\n", modulator, out); }
		//i++;

		pru_audio_write(out);
	}

	return 0;
}