#include "time_track_model.h"

#include <string.h>
#include <stdio.h>

static bool valid_label(uint8_t label)
{
    return label < TIME_TRACK_LABEL_COUNT;
}

static uint32_t day_index(uint32_t ts)
{
    return ts / TIME_TRACK_SECONDS_PER_DAY;
}

static void add_span_to_days(uint32_t today[TIME_TRACK_LABEL_COUNT],
                             uint32_t yesterday[TIME_TRACK_LABEL_COUNT],
                             uint32_t now_sec,
                             uint8_t label,
                             uint32_t start_ts,
                             uint32_t end_ts)
{
    if (!valid_label(label) || end_ts <= start_ts) {
        return;
    }

    const uint32_t now_day = day_index(now_sec);
    uint32_t cursor = start_ts;
    while (cursor < end_ts) {
        const uint32_t midnight = (day_index(cursor) + 1u) * TIME_TRACK_SECONDS_PER_DAY;
        const uint32_t chunk_end = end_ts < midnight ? end_ts : midnight;
        const uint32_t seconds = chunk_end - cursor;
        const uint32_t chunk_day = day_index(cursor);
        if (chunk_day == now_day) {
            today[label] += seconds;
        } else if (chunk_day + 1u == now_day) {
            yesterday[label] += seconds;
        }
        cursor = chunk_end;
    }
}

static void commit_open_session(time_track_state_t *state)
{
    if (!state->session.running) {
        return;
    }
    add_span_to_days(state->today, state->yesterday, state->now_sec,
                     state->session.label, state->session.start_ts, state->now_sec);
    state->session.running = false;
    state->session.start_ts = 0;
}

static void wr_u32(uint8_t **cursor, uint32_t value)
{
    (*cursor)[0] = (uint8_t)value;
    (*cursor)[1] = (uint8_t)(value >> 8);
    (*cursor)[2] = (uint8_t)(value >> 16);
    (*cursor)[3] = (uint8_t)(value >> 24);
    *cursor += 4;
}

static uint32_t rd_u32(const uint8_t **cursor)
{
    uint32_t value = (uint32_t)(*cursor)[0]
                   | ((uint32_t)(*cursor)[1] << 8)
                   | ((uint32_t)(*cursor)[2] << 16)
                   | ((uint32_t)(*cursor)[3] << 24);
    *cursor += 4;
    return value;
}

void time_track_init(time_track_state_t *state)
{
    if (!state) {
        return;
    }
    memset(state, 0, sizeof(*state));
}

bool time_track_advance(time_track_state_t *state, uint32_t now_sec)
{
    if (!state) {
        return false;
    }
    if (now_sec < state->now_sec) {
        now_sec = state->now_sec;
    }

    uint32_t old_day = day_index(state->now_sec);
    const uint32_t new_day = day_index(now_sec);
    state->now_sec = now_sec;

    bool rolled = false;
    while (old_day < new_day) {
        memcpy(state->yesterday, state->today, sizeof(state->today));
        memset(state->today, 0, sizeof(state->today));
        old_day++;
        rolled = true;
    }
    return rolled;
}

void time_track_handle(time_track_state_t *state, time_track_cmd_t cmd)
{
    if (!state) {
        return;
    }

    if (state->page == TIME_TRACK_PAGE_TIMER) {
        if (cmd == TIME_TRACK_CMD_UP || cmd == TIME_TRACK_CMD_DOWN) {
            if (state->session.running) {
                return;
            }
            const int delta = (cmd == TIME_TRACK_CMD_DOWN) ? 1 : TIME_TRACK_LABEL_COUNT - 1;
            state->selected_label = (uint8_t)((state->selected_label + delta) % TIME_TRACK_LABEL_COUNT);
            return;
        }
        if (cmd == TIME_TRACK_CMD_OK) {
            if (state->session.running) {
                commit_open_session(state);
            } else {
                state->session.running = true;
                state->session.label = state->selected_label;
                state->session.start_ts = state->now_sec;
            }
            return;
        }
        if (cmd == TIME_TRACK_CMD_OK_LONG) {
            state->page = TIME_TRACK_PAGE_TODAY;
            state->day_view = TIME_TRACK_DAY_TODAY;
        }
        return;
    }

    if (cmd == TIME_TRACK_CMD_UP || cmd == TIME_TRACK_CMD_DOWN) {
        state->day_view = (state->day_view == TIME_TRACK_DAY_TODAY)
                        ? TIME_TRACK_DAY_YESTERDAY
                        : TIME_TRACK_DAY_TODAY;
        return;
    }
    if (cmd == TIME_TRACK_CMD_OK_LONG) {
        state->page = TIME_TRACK_PAGE_TIMER;
    }
}

void time_track_recover_open(time_track_state_t *state)
{
    if (!state) {
        return;
    }
    commit_open_session(state);
}

uint8_t time_track_active_label(const time_track_state_t *state)
{
    if (!state) {
        return 0;
    }
    if (state->session.running && valid_label(state->session.label)) {
        return state->session.label;
    }
    return valid_label(state->selected_label) ? state->selected_label : 0;
}

uint32_t time_track_elapsed(const time_track_state_t *state)
{
    if (!state || !state->session.running) {
        return 0;
    }
    if (state->now_sec < state->session.start_ts) {
        return 0;
    }
    return state->now_sec - state->session.start_ts;
}

void time_track_view_totals(const time_track_state_t *state,
                            time_track_day_view_t view,
                            uint32_t out[TIME_TRACK_LABEL_COUNT])
{
    if (!out) {
        return;
    }
    memset(out, 0, sizeof(uint32_t) * TIME_TRACK_LABEL_COUNT);
    if (!state) {
        return;
    }

    const uint32_t *base = (view == TIME_TRACK_DAY_YESTERDAY) ? state->yesterday : state->today;
    memcpy(out, base, sizeof(uint32_t) * TIME_TRACK_LABEL_COUNT);

    if (!state->session.running) {
        return;
    }

    uint32_t live_today[TIME_TRACK_LABEL_COUNT] = {0};
    uint32_t live_yesterday[TIME_TRACK_LABEL_COUNT] = {0};
    add_span_to_days(live_today, live_yesterday, state->now_sec,
                     state->session.label, state->session.start_ts, state->now_sec);
    const uint32_t *live = (view == TIME_TRACK_DAY_YESTERDAY) ? live_yesterday : live_today;
    for (int i = 0; i < TIME_TRACK_LABEL_COUNT; i++) {
        out[i] += live[i];
    }
}

uint32_t time_track_total(const uint32_t values[TIME_TRACK_LABEL_COUNT])
{
    if (!values) {
        return 0;
    }
    uint32_t total = 0;
    for (int i = 0; i < TIME_TRACK_LABEL_COUNT; i++) {
        total += values[i];
    }
    return total;
}

uint16_t time_track_bar_px(uint32_t seconds, uint32_t total, uint16_t max_px)
{
    if (seconds == 0 || total == 0 || max_px == 0) {
        return 0;
    }
    const uint32_t px = (seconds * (uint32_t)max_px) / total;
    if (px == 0) {
        return 1;
    }
    if (px > max_px) {
        return max_px;
    }
    return (uint16_t)px;
}

void time_track_format_hms(uint32_t seconds, char *buf, size_t buf_size)
{
    if (!buf || buf_size == 0) {
        return;
    }
    const unsigned hours = (unsigned)(seconds / 3600u);
    const unsigned minutes = (unsigned)((seconds % 3600u) / 60u);
    const unsigned secs = (unsigned)(seconds % 60u);
    snprintf(buf, buf_size, "%02u:%02u:%02u", hours, minutes, secs);
}

void time_track_format_hm(uint32_t seconds, char *buf, size_t buf_size)
{
    if (!buf || buf_size == 0) {
        return;
    }
    snprintf(buf, buf_size, "%uh %um",
             (unsigned)(seconds / 3600u),
             (unsigned)((seconds % 3600u) / 60u));
}

size_t time_track_export(const time_track_state_t *state, uint8_t *buf, size_t buf_size)
{
    if (!state || !buf || buf_size < TIME_TRACK_STORE_SIZE) {
        return 0;
    }

    uint8_t *cursor = buf;
    wr_u32(&cursor, TIME_TRACK_STORE_MAGIC);
    wr_u32(&cursor, state->now_sec);
    for (int i = 0; i < TIME_TRACK_LABEL_COUNT; i++) {
        wr_u32(&cursor, state->today[i]);
    }
    for (int i = 0; i < TIME_TRACK_LABEL_COUNT; i++) {
        wr_u32(&cursor, state->yesterday[i]);
    }
    *cursor++ = state->selected_label;
    *cursor++ = state->session.running ? 1u : 0u;
    *cursor++ = state->session.label;
    *cursor++ = 0;
    wr_u32(&cursor, state->session.start_ts);
    return TIME_TRACK_STORE_SIZE;
}

bool time_track_import(time_track_state_t *state, const uint8_t *buf, size_t buf_size)
{
    if (!state || !buf || buf_size < TIME_TRACK_STORE_SIZE) {
        return false;
    }

    const uint8_t *cursor = buf;
    if (rd_u32(&cursor) != TIME_TRACK_STORE_MAGIC) {
        return false;
    }

    time_track_init(state);
    state->now_sec = rd_u32(&cursor);
    for (int i = 0; i < TIME_TRACK_LABEL_COUNT; i++) {
        state->today[i] = rd_u32(&cursor);
    }
    for (int i = 0; i < TIME_TRACK_LABEL_COUNT; i++) {
        state->yesterday[i] = rd_u32(&cursor);
    }

    const uint8_t selected = *cursor++;
    const uint8_t running = *cursor++;
    const uint8_t label = *cursor++;
    cursor++; /* reserved */
    const uint32_t start_ts = rd_u32(&cursor);

    if (!valid_label(selected) || (running && !valid_label(label))) {
        time_track_init(state);
        return false;
    }

    state->selected_label = selected;
    state->session.running = running != 0;
    state->session.label = running ? label : selected;
    state->session.start_ts = running ? start_ts : 0;
    return true;
}
