/**
 * menu.c: determines which "sub-program" to run.
 * 
 * This is so that we can easily run individual parts of the code offline, such
 * as the vocoder.
 */

#include <string.h>

extern int main_ov(int argc, char **argv);

extern int main_app(int argc, char **argv);

int
main(int argc, char **argv) {
	if(argc <= 1) {
#ifdef HARDWARE
		main_app(argc, argv);
#else
		puts("not on hardware: you must specify -ov");
		return 1;
#endif
	}

	if(!strcmp(argv[1], "-ov")) {
		return main_ov(argc, argv);
	}

	printf("error: unknown option %s. available options are: -ov", argv[1]);
	return 1;
}