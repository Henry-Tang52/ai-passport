#pragma once

#include "time_track_model.h"

void time_track_ui_create(void);
void time_track_ui_refresh(const time_track_state_t *state, int battery_soc);
void time_track_ui_destroy(void);
