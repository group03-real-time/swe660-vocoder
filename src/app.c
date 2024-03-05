#include "app.h"
#include "types.h"

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/utsname.h>

bool app_running = false;

_Noreturn void 
app_fatal_error(const char *summary) {
	/* This function just helps to print a message and exit() in one line of code */
	printf("fatal error: %s\n", summary);
	exit(-1);
}

/** Our signal handler for SIGINT and SIGTERM. Performs a graceful exit. */
static void
signal_handler(int_stdc signum) { /* Must use a standard int to fulfill ABI */
	if(signum == SIGINT || signum == SIGTERM)
		app_graceful_exit();
}

void
app_show_utsname(void) {
	struct utsname sys_info;
	uname(&sys_info);

/* Automatically include the TARGET_NAME in the utsname info if it is defined. */
#ifdef TARGET_NAME
	puts("--- " TARGET_NAME " utsname ---");
#else
	puts("--- utsname ---");
#endif
	printf("  System name: %s\n", sys_info.sysname);
	printf("  Node name: %s\n",   sys_info.nodename);
	printf("  Machine: %s\n",     sys_info.machine);
}

void
app_init_loop(void) {
	/* After this call, the loop should be running. Blocking functions are
	 * a bad idea as well once we set up the signal handlers. */
	app_running = true;

	/* Use sigaction API */
	struct sigaction sa = {0};
	sa.sa_handler = signal_handler;

	/* Same signal handler for SIGINT and SIGTERM */
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
}

void
app_graceful_exit(void) {
	/* For now... we don't need to do anything but change this value. */
	app_running = false;
}

void
app_sleep_ms(int32_t ms) {
	/* Input checking */
	if(ms <= 0) return;
	/* app_sleep_ms() only sleeps if the app is running */
	if(!app_running) return;

	/* Compute a timespec for nanosleep */
	struct timespec tv = {0};
	tv.tv_sec = ms / 1000;

	const int32_t leftover_ms = (ms % 1000);
	tv.tv_nsec = leftover_ms * 1000000;

	struct timespec rem = {0};

	int_stdc res = 0;
	
	/** 
	 * check for and handle interrupts other than Ctrl-C 
	 * if other interrupts returned from nanosleep() set time remaining (tv) to remaining time 
	 * and loop through again
	 */
	do {
		res = nanosleep(&tv, &rem);

		/* If !app_running, that means we hit Ctrl-C and our signal handler
		 * gracefully exited. So stop sleeping */
		if(!app_running) return;

		if(res == EINTR) {
			tv = rem;
			rem = (struct timespec){0};
		}
	} while(res == EINTR);
}
