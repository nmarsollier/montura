/* Mount - mount.c
 *
 * Purpose: core mount state and synchronization helpers.
 */
#include "mount.h"
#include "mount_internal.h"

#include <stdio.h>
#include <string.h>

#include "esp_timer.h"
#include "motors.h"

MountSettings mount_internal_state;

/*
 * Business use case: initialize the mount at startup.
 *
 * Objective: load persisted configuration and leave the mount ready for remote
 * operation and runtime scheduling.
 */
void mount_init(void) {
    mount_settings_storage_load(&mount_internal_state);
}
