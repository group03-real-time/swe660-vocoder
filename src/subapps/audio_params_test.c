#include "audio_params.h"

int
main_apt(int argc, char **argv) {
	audio_params params;
	audio_params_default(&params);
	audio_params_init_multiplexer();

	for(;;) {
		audio_params_tick_multiplexer(&params, true);
	}
}