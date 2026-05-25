idf_component_register(
        SRCS
        "main.c"

        "mount/mount.c"
        "mount/mount_tools.c"
        "mount/mount_tracking.c"
        "mount/mount_goto.c"
        "mount/mount_stop.c"
        "mount/mount_park.c"
        "mount/mount_unpark.c"
        "mount/mount_sync.c"
        "mount/mount_home.c"
        "mount/mount_settings.c"
        "mount/mount_time.c"
        "mount/mount_visible_status.c"


        "mount/mount_coordinates.c"
        "mount/mount_settings_storage.c"

        "rest/rest_server.c"
        "tools/rest_send_result.c"
        "rest/rest_goto_handler.c"
        "rest/rest_move_axis_handler.c"
        "rest/rest_home_handler.c"
        "mount/mount_move_axis.c"
        "rest/rest_park_handler.c"
        "rest/rest_screen_handler.c"
        "rest/rest_settings_handler.c"
        "rest/rest_status_handler.c"
        "rest/rest_stop_handler.c"
        "rest/rest_tracking_handler.c"
        "rest/rest_sync_handler.c"
        "rest/rest_unpark_handler.c"
        "rest/rest_wifi_handler.c"

        "screen/screen.c"
        "screen/screen_ssd1306.c"
        "screen/screen_draw_bitmap.c"
        "screen/screen_font_5x10.c"
        "screen/screen_font_8x12.c"

        "motors/motors_init.c"
        "motors/motors_tools.c"
        "motors_motion/motors_motion_task.c"
        "motors_motion/motors_motion_sync.c"
        "motors_motion/motors_motion_hw.c"
        "motors/motors_get_slewing_speed.c"
        "motors/motors_home.c"
        "motors/motors_button_stop.c"
        "motors/motors_sync_position.c"
        "motors/motors_get_tracking_speed.c"
        "motors/motors_start_tracking.c"
        "motors/motors_stop.c"
        "motors/motors_park.c"
        "motors/motors_enable.c"
        "motors/motors_disable.c"
        "motors/motors_set_axis_velocity.c"
        "motors/motors_current_state.c"
        "motors/motors_slew_axis.c"
        "motors/motors_slew_to_angle.c"
        "buttons/buttons_poll.c"
        "buttons/buttons_hw.c"

        "tmc/tmc_init.c"
        "tmc/tmc_get_active_microsteps.c"

        "runtime/runtime_setup.c"
        "runtime/runtime_config.c"
        "runtime/runtime_loop.c"

        "tools/http_response.c"
        "tools/json_utils.c"
        "tools/number_utils.c"
        "tools/string_utils.c"

        "network/network.c"
        "network/network_configure_home_wifi.c"
        "network/network_is_setup_ap_started.c"
        "network/network_ip.c"

        INCLUDE_DIRS
        "."
        "mount"
        "rest"
        "screen"
        "motors"
        "motors_motion"
        "buttons"
        "runtime"
        "tools"
        "network"
        "tmc"

        EMBED_TXTFILES
        "../www/dist/index.html"

        REQUIRES
        esp_http_server
        esp_netif
        esp_event
        esp_wifi
        esp_timer
        nvs_flash
        esp_driver_i2c
        esp_driver_gpio
        esp_driver_uart
)
