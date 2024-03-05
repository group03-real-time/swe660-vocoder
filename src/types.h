#ifndef TYPES_H
#define TYPES_H

/* A helper file to make it convenient to use standard int types and other
 * similar typedefs. */

#include <stdbool.h>
#include <stdint.h>

/* Typedef for file descriptors, for two reasons:
 * 1. Makes MISRA slightly happier.
 * 2. Makes it clear to the code reader that this int variable is an fd,
 *    specifically, which has its own conventions (e.g. -1 is invalid).
 */
typedef int int_fd;

/* For other kinds of functions that return or otherwise require ints. Make it
 * clear we want a C int specifically. */
typedef int int_stdc;

#endif