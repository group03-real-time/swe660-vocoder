#include "buttons.h"
#include "dsp/synth.h"

int
main_bh(int argc, char **argv) {
    init_button_arr();
    synth *syn;
    
    for(;;) {
        button_tick(syn, true);
    }
}
