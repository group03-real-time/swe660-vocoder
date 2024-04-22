#ifndef MMAP_H
#define MMAP_H

#include "types.h"

/**
 * Gets a memory address corresponding to the physical memory address "key," with
 * the given 'size' for the entire mapping.
 * 
 * Will return an existing mapping if one with the given key already exists. 
 * Otherwise, creates a new mapping using mmap, and then stores it for future
 * lookups.
 * 
 * Note that, as such, any call to this function with the same key should also
 * specify the same size.
 */
void* mmap_get_mapping(uintptr_t key, size_t size);

/**
 * Writes a string to the file at the given file path.
 * 
 * Intended for use writing to files under the sysfs. In particular, the usage
 * of this function is kind of similar to, for example,
 * 
 *     echo start > /sys/class/remoteproc/remoteproc1/state
 * 
 * But in a form that can be used in C.
 */
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