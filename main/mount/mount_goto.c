/* Mount - mount_goto.c
 *
 * Purpose: move the mount to a requested sky position.
 */
#include "mount.h"
#include "mount_internal.h"

#include "motors.h"

MountResult mount_goto(float ra, float dec, int speed_rate) {
    EquatorialCoordinates eq = {
        .ra_hours = ra,
        .dec_deg = dec
    };

    AxisCoordinates axis = equatorial_to_axis(eq);

    MotorResultCode rc1 = motors_slew_to_angle(axis.ra_axis_deg, axis.dec_axis_deg, speed_rate, mount_internal_state.lat);

    if (rc1 != MOTOR_OK) {
        return mount_result_error("Failed to start GOTO");
    }

    return mount_result_ok();
}
