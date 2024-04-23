#include "audio_params.h"

#include <unistd.h> /* usleep */

int
main_apt(int argc, char **argv) {
	audio_params params;
	audio_params_default(&params);
	audio_params_init_multiplexer();

	for(;;) {
		audio_params_tick_multiplexer(&params, true);
		usleep(30); /* must sleep to give multiplexer time to read next data */
	}
}