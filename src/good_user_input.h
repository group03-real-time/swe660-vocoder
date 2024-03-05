#ifndef GOOD_USER_INPUT_H
#define GOOD_USER_INPUT_H

#include "types.h"

/* See good_user_input.c for Rationale */

/** The input was invalid, e.g. 'abc' when expecting an integer. */
#define INPUT_INVALID    0
/** The input was valid. May require further validation from the user. */
#define INPUT_VALID      1
/** 
 * The input encountered EOF. In this case, no future input_ reads will
 * NOT result in EOF, if the underlying I/O system is implemented reasonably well.
 */
#define INPUT_EOF        2
/** The input was an empty line. */
#define INPUT_EMPTY_LINE 3

/** 
 * A type representing the input result for each input_read_x function. 
 * Should only ever use the INPUT_X constants. Not an enum because those are not
 * super great in C11.
 */
typedef int32_t input_result;

/**
 * Reads an integer from standard input and stores it in the given output pointer,
 * if not NULL.
 * Returns an input_result representing whether the input was valid or whether
 * an EOF, empty line, or invalid input was encountered.
 */
input_result input_read_int(int32_t *output);

/**
 * Reads a float from standard input and stores it in the given output pointer,
 * if not NULL.
 * Returns an input_result representing whether the input was valid or whether
 * an EOF, empty line, or invalid input was encountered.
 */
input_result input_read_float(float *output);

/**
 * Returns whether the input is at EOF.
 */
bool input_is_eof(void);

#endif