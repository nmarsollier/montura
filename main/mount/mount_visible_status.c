#include "mount.h"
#include "mount_internal.h"

#include "motors.h"

/*
 * Business use case: expose the visible operational status.
 *
 * Objective: provide the UI and API with a consistent view of the mount for
 * observability and external decision-making.
 */
VisibleStatusData mount_get_visible_status_data(void) {
    MotorsState pos = motors_current_state();
    AxisCoordinates axis = {
        .ra_axis_deg = pos.ra_position,
        .dec_axis_deg = pos.dec_position
    };

    EquatorialCoordinates eq = axis_to_equatorial(axis);

    return (VisibleStatusData)
    {
        .
        status = pos.status,
        .
        tracking = pos.tracking,
        .
        ra = ra_hours_to_hms(eq.ra_hours),
        .
        dec = dec_deg_to_dms(eq.dec_deg),
        .
        last_update = pos.last_update,
        .
        settings = mount_internal_state,
    };
}
