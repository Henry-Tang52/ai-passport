#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TIME_TRACK_LABEL_COUNT 6
#define TIME_TRACK_SECONDS_PER_DAY 86400u
#define TIME_TRACK_STORE_MAGIC 0x31544B54u /* TKT1 */
#define TIME_TRACK_STORE_SIZE 64

typedef enum {
    TIME_TRACK_LABEL_INTERNSHIP = 0,
    TIME_TRACK_LABEL_APPLY,
    TIME_TRACK_LABEL_LIFE,
    TIME_TRACK_LABEL_INTERVIEW,
    TIME_TRACK_LABEL_SCHOOL,
    TIME_TRACK_LABEL_SPORT,
} time_track_label_t;

typedef enum {
    TIME_TRACK_PAGE_TIMER = 0,
    TIME_TRACK_PAGE_TODAY,
} time_track_page_t;

typedef enum {
    TIME_TRACK_DAY_TODAY = 0,
    TIME_TRACK_DAY_YESTERDAY,
} time_track_day_view_t;

typedef enum {
    TIME_TRACK_CMD_UP = 0,
    TIME_TRACK_CMD_DOWN,
    TIME_TRACK_CMD_OK,
    TIME_TRACK_CMD_OK_LONG,
} time_track_cmd_t;

typedef struct {
    bool running;
    uint8_t label;
    uint32_t start_ts;
} time_track_session_t;

typedef struct {
    uint32_t now_sec;
    uint32_t today[TIME_TRACK_LABEL_COUNT];
    uint32_t yesterday[TIME_TRACK_LABEL_COUNT];
    uint8_t selected_label;
    time_track_session_t session;
    time_track_page_t page;
    time_track_day_view_t day_view;
} time_track_state_t;

void time_track_init(time_track_state_t *state);
bool time_track_advance(time_track_state_t *state, uint32_t now_sec);
void time_track_handle(time_track_state_t *state, time_track_cmd_t cmd);
void time_track_recover_open(time_track_state_t *state);

uint8_t time_track_active_label(const time_track_state_t *state);
uint32_t time_track_elapsed(const time_track_state_t *state);
void time_track_view_totals(const time_track_state_t *state,
                            time_track_day_view_t view,
                            uint32_t out[TIME_TRACK_LABEL_COUNT]);
uint32_t time_track_total(const uint32_t values[TIME_TRACK_LABEL_COUNT]);
uint16_t time_track_bar_px(uint32_t seconds, uint32_t total, uint16_t max_px);

void time_track_format_hms(uint32_t seconds, char *buf, size_t buf_size);
void time_track_format_hm(uint32_t seconds, char *buf, size_t buf_size);

size_t time_track_export(const time_track_state_t *state, uint8_t *buf, size_t buf_size);
bool time_track_import(time_track_state_t *state, const uint8_t *buf, size_t buf_size);
