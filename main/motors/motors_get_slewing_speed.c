/* Motors - motors_get_slewing_speed.c
 *
 * Purpose: return the default slewing speed for a profile.
 */
#include "motors.h"

/*
 * Business-facing utility returning the default slewing angular speed for the
 * current motion profile.
 */
float motors_get_slewing_speed(int speed) {
    switch (speed) {
        case 1: return 0.5f;
        case 2: return 1;
        case 3: return 16;
        default: return 32;
    }
}
