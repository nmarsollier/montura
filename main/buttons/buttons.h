#pragma once

#include <stdbool.h>

/* Poll inputs and dispatch button actions on rising edges. */
void buttons_poll_inputs(void);

bool buttons_hw_is_stop_pressed(void);

bool buttons_hw_is_home_pressed(void);

void buttons_hw_init(void);


void buttons_init(void);
