/* Motors - motors_home.c
 *
 * Purpose: move the mount to the home position.
 */
#include "motors.h"

void motors_home(void) {
    motors_stop();

    motors_slew_to_angle(0.0f, 0.0f, 0);
}
