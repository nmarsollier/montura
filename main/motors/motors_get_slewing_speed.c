/* Motors - motors_get_slewing_speed.c
 *
 * Purpose: return the default slewing speed for a profile.
 */
#include "motors.h"

/*
 * Map a speed profile number to an axis angular velocity in deg/s.
 */
float motors_get_slewing_speed(int speed) {
    switch (speed) {
        case 1: return 2.0f;
        case 2: return 4.0f;
        case 3: return 8.0f;
        default: return 16.0f;
    }
}
