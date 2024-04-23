#include "buttons.h"
#include "dsp/synth.h"

int
main_bh(int argc, char **argv) {
    init_button_arr();

    synth syn;
	synth_init(&syn);
    
    for(;;) {
		for(int i = 0; i < BUTTON_COUNT; ++i) {
       		button_tick(&syn, i, true);
		}
    }
}
