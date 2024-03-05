#include "gpio.h"

/* Implemented for both emulator and hardware */

/* There doesn't seem to be any rhyme or reason to the pattern in the GPIO pins.
 * For now, this table of valid pins was just compiled base on ls /sys/class/gpio. */
bool
gpio_pin_valid(int32_t pin_number) {
	switch(pin_number) {
		case 2:
		case 3:
		case 4:
		case 5:
		/* no 6 */
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		/* no 16-19 */
		case 20:
		/* no 21 */
		case 22:
		case 23:
		/* no 24-25 */
		case 26:
		case 27:
		/* no 28-29 */
		case 30:
		case 31:
		case 32:
		case 33:
		case 34:
		case 35:
		case 36:
		case 37:
		case 38:
		case 39:
		/* no 40-43 */
		case 44:
		case 45:
		case 46:
		case 47:
		case 48:
		case 49:
		case 50:
		case 51:
		/* no 52-59 */
		case 60:
		case 61:
		case 62:
		case 63:
		/* no 64 */
		case 65:
		case 66:
		case 67:
		case 68:
		case 69:
		case 70:
		case 71:
		case 72:
		case 73:
		case 74:
		case 75:
		case 76:
		case 77:
		case 78:
		case 79:
		case 80:
		case 81:
		/* no 82-85 */
		case 86:
		case 87:
		case 88:
		case 89:
		/* no 90-109 */
		case 110:
		case 111:
		case 112:
		case 113:
		case 114:
		case 115:
		case 116:
		case 117:
			return true;
		default:
			return false;
	}
}