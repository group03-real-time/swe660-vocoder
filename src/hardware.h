#ifndef MMAP_H
#define MMAP_H

#include "types.h"

void* mmap_get_mapping(uintptr_t key, size_t size);

int sysfs_write_string(const char *filepath, const char *string);

/**
 * Initializes all hardware resources (including the GPIO subsystem and the
 * PRU drivers). Allows code to very easily use the gpio_ and pru_ functions
 * without worrying about initializing, as hardware_init() is called in
 * menu.c as soon as the code boots up.
 */
void hardware_init();

/**
 * Cleans up hardware resources, including any memory mappings, and stops the
 * PRUs. Does not write to any GPIO pins, so that will have to be done separately
 * if you want pins to stop with a certain value written.
 */
void hardware_shutdown();

#endif