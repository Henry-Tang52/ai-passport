#pragma once

#include "time_track_model.h"

#include "esp_err.h"

esp_err_t time_track_store_init(void);
esp_err_t time_track_store_load(time_track_state_t *state);
esp_err_t time_track_store_save(const time_track_state_t *state);
