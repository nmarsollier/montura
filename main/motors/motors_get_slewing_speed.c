/* Motors - motors_get_slewing_speed.c
 *
 * Purpose: return the default slewing speed for a profile.
 */
#include "motors.h"

/*
 * Given a number between 1..4, Returns degrees per second speed used on slew
 */
float motors_get_slewing_speed(int speed) {
    switch (speed) {
        case 1: return 2.0f;
        case 2: return 4.0f;
        case 3: return 6.0f;
        default: return 8.0f;
    }
}
