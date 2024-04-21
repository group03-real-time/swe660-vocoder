/**
 * menu.c: determines which "sub-program" to run.
 * 
 * This is so that we can easily run individual parts of the code offline, such
 * as the vocoder.
 */

#include <string.h>
#include <stdio.h>

#include "hardware.h" /* For init and shutdown */

extern int main_ov(int argc, char **argv);
extern int main_os(int argc, char **argv);
extern int main_ovs(int argc, char **argv);

extern int main_ppw(int argc, char **argv);
extern int main_prw(int argc, char **argv);
extern int main_bwt(int argc, char **argv);

extern int main_app(int argc, char **argv);

/* Helper macro to ensure that hardware-related subapps always get proper
 * initialization and shutdown. */
#define HARDWARE_SUBAPP(app_name)\
do {\
	hardware_init();\
	int return_value = app_name(argc, argv);\
	hardware_shutdown();\
	return return_value;\
} while(0)

int
main(int argc, char **argv) {
	if(argc <= 1) {
#ifdef HARDWARE
		/* On hardware, running with 0 arguments just starts the main functionality. */
		HARDWARE_SUBAPP(main_app);
#else
		puts("not on hardware: you must specify -ov, -os, -ovs or -help");
		return 1;
#endif
	}

	if(!strcmp(argv[1], "-ov")) {
		return main_ov(argc, argv);
	}

	if(!strcmp(argv[1], "-os")) {
		return main_os(argc, argv);
	}

	if(!strcmp(argv[1], "-ovs")) {
		return main_ovs(argc, argv);
	}

	if(!strcmp(argv[1], "-help")) {
		puts("possible options:\n"
		"  -ov: 'offline vocode': run the vocoder on a modulator.wav and carrier.wav, producing an output.wav\n"
		"  -os: 'offline synth': run the synthesizer and create an output.wav\n"
		"  -ovs: 'offline vocoder synth': run the vocoder on a modulator.wav and the built-in synth, producing an output.wav\n"
		"  -help: show this help menu\n"
		"if you are on hardware, some additional options are available:\n"
		"  -ppw: 'PRU play wav': use the PRU audio setup to play a WAV file over i2s\n"
		"  -prw: 'PRU record wav': use the PRU audio/sampling setup to record a WAV file over the ADC pin 0\n"
		"  -bg: 'button grid': run an experimental 'button grid' setup to test it\n"
		"  -bwt: 'button wiring test': tests to make sure all button GPIO pins can be read\n"
		);
		return 0;
	}

/* These options only work on hardware */
#ifdef HARDWARE
	if(!strcmp(argv[1], "-ppw")) {
		HARDWARE_SUBAPP(main_ppw);
	}

	if(!strcmp(argv[1], "-prw")) {
		HARDWARE_SUBAPP(main_prw);
	}

	if(!strcmp(argv[1], "-bwt")) {
		HARDWARE_SUBAPP(main_bwt);
	}
#endif

	printf("error: unknown option %s. run -help to see available options\n", argv[1]);
	return 1;
}