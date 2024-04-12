/**
 * menu.c: determines which "sub-program" to run.
 * 
 * This is so that we can easily run individual parts of the code offline, such
 * as the vocoder.
 */

#include <string.h>
#include <stdio.h>

extern int main_ov(int argc, char **argv);
extern int main_os(int argc, char **argv);
extern int main_ovs(int argc, char **argv);

extern int main_ppw(int argc, char **argv);
extern int main_prw(int argc, char **argv);

extern int main_bg(int argc, char **argv);

extern int main_app(int argc, char **argv);

extern void hardware_init();

int
main(int argc, char **argv) {
#ifdef HARDWARE
	/* Ensure PRU's (and mmap) is initialized when we're on the hardware. */
	hardware_init();
#endif

	if(argc <= 1) {
#ifdef HARDWARE
		return main_app(argc, argv);
#else
		puts("not on hardware: you must specify -ov, -os, -ovs, or -ppw");
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

	if(!strcmp(argv[1], "-ppw")) {
		return main_ppw(argc, argv);
	}

	if(!strcmp(argv[1], "-prw")) {
		return main_prw(argc, argv);
	}

	if(!strcmp(argv[1], "-bg")) {
		return main_bg(argc, argv);
	}

	printf("error: unknown option %s. available options are: -ov, -os, -ovs", argv[1]);
	return 1;
}