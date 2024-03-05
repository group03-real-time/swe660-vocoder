#include "good_user_input.h"

#include <stdio.h>

/* --- Rationale ---
 *
 * Basically, the situation for user input is as follows:
 * - scanf won't tell us when someone enters an empty line. This is very
 *   problematic.
 * - it also doesn't seem to be very possible to detect EOF when using scanf.
 *   this is workable, but still annoying.
 * As such, the only real way to do user input is to fgets() and then sscanf().
 * But annoyingly, even this doesn't work right, because fgets() means that 
 * either we eat the whole line and it fits in our buffer, or it doesn't fit
 * in our buffer and so then we don't even get a whole line and have to call
 * it again to get a newline.
 * 
 * And, we are on an embedded system, so it's bad practice to do the normal thing
 * of just keep allocating more string space until we can fit the whole line.
 * 
 * So instead of doing any of that, just have a fixed-size buffer, and throw
 * away any input up to the next newline that doesn't fit. Simple, no memory
 * bugs, no weird behavior, works fine for any reasonable user, and any user
 * who wants to input more than 128 characters will simply get a warning without
 * any further interruption.
 * 
 * Whatever they were trying to input was probably incorrect anyways.
 */

/* Here, use one fixed-length static buffer. We aren't multithreaded, so
 * we can re-use the same buffer without even locking.
 *
 * This is to save memory. If we weren't concerned about memory, we would just
 * use a stack-allocated buffer or similar. */
#define INPUT_BUF_LENGTH 128
static char input_buf[INPUT_BUF_LENGTH];

static bool
input_read_into_buffer(void) {
	int32_t line_len = 0;
	do {
		int_stdc next = fgetc(stdin);
		if(next == EOF) {
			/* If we hit EOF with an empty line, we will never get valid input here. */
			if(line_len == 0) return true;

			/* Otherwise, we're done reading the line. Skip the line eating. */
			goto has_full_line;
		}

		if(next == '\n') {
			/* If we see a '\n', we're done. Skip the line eating code with goto. */
			goto has_full_line;
		}

		input_buf[line_len++] = (char)next;
	} while(line_len < INPUT_BUF_LENGTH);

	/* If we get here, it means we didn't see a '\n' or EOF. So, print a warning 
	 * to the user. */
	printf("WARNING: ignoring all input past column %d\n", INPUT_BUF_LENGTH - 1);

	/* In order to actually eat the whole line, we need to just read until we
	 * see '\n' or EOF. */
	for(;;) {
		int_stdc next = fgetc(stdin);
		if(next == '\n' || next == EOF) {
			break;
		}
	}

has_full_line:
	input_buf[line_len] = '\0';

	/* We didn't hit EOF yet. */
	return false;
}

input_result
input_read_int(int32_t *output) {
	/* If this function returns true, we got an EOF */
	if(input_read_into_buffer()) return INPUT_EOF;

	/* If the first character is zero, we got an empty line */
	if(input_buf[0] == '\0') return INPUT_EMPTY_LINE;

	/* Use sscanf to do the number parsing from the buffer. Can result in
	 * INPUT_INVALID if the buffer did not contain an integer. */
	int32_t result = 0;
	if(sscanf(input_buf, "%d", &result) != 1) return INPUT_INVALID;

	/* Only write to the output pointer if it's provided. This function can
	 * also be used to, I guess, do a dry run..? */
	if(output) *output = result;

	/* In this case we wrote to the output and it was valid. The user may 
	 * of course perform further validation (e.g. is it positive). */
	return INPUT_VALID;
}

input_result
input_read_float(float *output) {
	/* Same idea as input_read_int. */
	if(input_read_into_buffer()) return INPUT_EOF;

	if(input_buf[0] == '\0') return INPUT_EMPTY_LINE;

	/* Use a float reuslt and %f instead of an int result and %d. */
	float result = 0;
	if(sscanf(input_buf, "%f", &result) != 1) return INPUT_INVALID;

	if(output) *output = result;
	return INPUT_VALID;
}

bool
input_is_eof(void) {
	/* Can be used to make input loops look a bit nicer, e.g. so the loop can
	 * call input_read() and input_is_eof() instead of input_read() and feof(). */
	return !!feof(stdin);
}
