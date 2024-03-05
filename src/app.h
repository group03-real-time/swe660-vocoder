#ifndef APP_H
#define APP_H

#include "types.h"

/* app.h: contains some common functionality useful for building embedded
 * applications */

/**
 * Prints the utsname info for the application. Intended to be run on startup.
 */
void app_show_utsname(void);

/**
 * Begins an app main loop related to the bool app_running. Essentially, the
 * loop will run until something calls app_graceful_exit. Note that after
 * app_init_loop() is called, the program can no longer be killed with Ctrl-C--
 * instead, Ctrl-C will simply result in app_running being set to false.
 * 
 * So, in order to kill the app, it must sychronously check app_running and then
 * stop whatever it is doing.
 * 
 * That is, running a blocking call like fgetc() or similar after app_init_loop()
 * is a bad idea because it won't be possible to nicely exit out of it.
 */
void app_init_loop(void);

/**
 * Causes the app_running boolean to become false and performs any other associated
 * cleanup. Called by the Ctrl-C signal handler after app_init_loop() has been
 * executed.
 */
void app_graceful_exit(void);

/** 
 * A sleep method that will gracefully stop sleeping as soon as app_graceful_exit()
 * has been called. That is, this sleep may be interrupted by Ctrl-C, with the
 * intent of exiting the app.
 */
void app_sleep_ms(int32_t ms);

/**
 * Causes the application to exit with a fatal error message. This is useful when
 * we are trying to handle an error where we have no real graceful way to recover.
 */
_Noreturn void app_fatal_error(const char *summary);

/**
 * A boolean representing whether the app is still running. Should be used as
 * a loop condition for the app's main loop, after calling app_init_loop().
 */
extern bool app_running;

#endif