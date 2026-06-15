/* Motors - motors_home.c
 *
 * Purpose: move the mount to the home position.
 *
 * Sends a high-priority SLEW to (0°, 0°) which preempts any motion
 * in progress — no need for an explicit STOP first (that would race
 * with the SLEW in the queue and cancel it).
 */
#include "motors.h"

void motors_home(float lat) {
    motors_slew_to_angle(0.0f, 0.0f, 0, lat);
}
