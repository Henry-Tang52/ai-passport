#include <assert.h>
#include <string.h>

#include "time_track_model.h"

static void test_start_stop_creates_record(void)
{
    time_track_state_t state;
    time_track_init(&state);
    time_track_advance(&state, 100);
    time_track_handle(&state, TIME_TRACK_CMD_DOWN);
    time_track_handle(&state, TIME_TRACK_CMD_DOWN);
    assert(time_track_active_label(&state) == TIME_TRACK_LABEL_LIFE);
    time_track_handle(&state, TIME_TRACK_CMD_OK);
    assert(state.session.running);
    time_track_advance(&state, 250);
    assert(time_track_elapsed(&state) == 150);
    time_track_handle(&state, TIME_TRACK_CMD_OK);
    assert(!state.session.running);
    assert(state.today[TIME_TRACK_LABEL_LIFE] == 150);
    assert(time_track_elapsed(&state) == 0);
}

static void test_cannot_change_label_while_running(void)
{
    time_track_state_t state;
    time_track_init(&state);
    time_track_handle(&state, TIME_TRACK_CMD_OK);
    const uint8_t started = time_track_active_label(&state);
    time_track_handle(&state, TIME_TRACK_CMD_UP);
    time_track_handle(&state, TIME_TRACK_CMD_DOWN);
    assert(time_track_active_label(&state) == started);
    assert(state.selected_label == started);
}

static void test_today_bars_and_totals(void)
{
    time_track_state_t state;
    time_track_init(&state);
    state.today[TIME_TRACK_LABEL_INTERNSHIP] = 3600;
    state.today[TIME_TRACK_LABEL_APPLY] = 1800;
    state.today[TIME_TRACK_LABEL_SPORT] = 600;

    uint32_t view[TIME_TRACK_LABEL_COUNT];
    time_track_view_totals(&state, TIME_TRACK_DAY_TODAY, view);
    const uint32_t total = time_track_total(view);
    assert(total == 6000);
    assert(time_track_bar_px(view[TIME_TRACK_LABEL_INTERNSHIP], total, 100) == 60);
    assert(time_track_bar_px(view[TIME_TRACK_LABEL_APPLY], total, 100) == 30);
    assert(time_track_bar_px(view[TIME_TRACK_LABEL_SPORT], total, 100) == 10);
    assert(time_track_bar_px(0, total, 100) == 0);
    assert(time_track_bar_px(1, 10000, 100) == 1);
}

static void test_switch_yesterday_view(void)
{
    time_track_state_t state;
    time_track_init(&state);
    state.today[TIME_TRACK_LABEL_SCHOOL] = 120;
    state.yesterday[TIME_TRACK_LABEL_SCHOOL] = 480;

    time_track_handle(&state, TIME_TRACK_CMD_OK_LONG);
    assert(state.page == TIME_TRACK_PAGE_TODAY);
    assert(state.day_view == TIME_TRACK_DAY_TODAY);
    time_track_handle(&state, TIME_TRACK_CMD_DOWN);
    assert(state.day_view == TIME_TRACK_DAY_YESTERDAY);
    time_track_handle(&state, TIME_TRACK_CMD_UP);
    assert(state.day_view == TIME_TRACK_DAY_TODAY);

    uint32_t view[TIME_TRACK_LABEL_COUNT];
    time_track_view_totals(&state, TIME_TRACK_DAY_YESTERDAY, view);
    assert(view[TIME_TRACK_LABEL_SCHOOL] == 480);
}

static void test_power_loss_recovers_open_session(void)
{
    time_track_state_t live;
    time_track_init(&live);
    time_track_advance(&live, 1000);
    live.selected_label = TIME_TRACK_LABEL_INTERVIEW;
    time_track_handle(&live, TIME_TRACK_CMD_OK);
    time_track_advance(&live, 1300);

    uint8_t blob[TIME_TRACK_STORE_SIZE];
    assert(time_track_export(&live, blob, sizeof(blob)) == TIME_TRACK_STORE_SIZE);

    time_track_state_t restored;
    assert(time_track_import(&restored, blob, sizeof(blob)));
    assert(restored.session.running);
    assert(restored.session.label == TIME_TRACK_LABEL_INTERVIEW);
    assert(restored.session.start_ts == 1000);
    assert(restored.now_sec == 1300);

    time_track_recover_open(&restored);
    assert(!restored.session.running);
    assert(restored.today[TIME_TRACK_LABEL_INTERVIEW] == 300);
}

static void test_midnight_split_and_day_roll(void)
{
    time_track_state_t state;
    time_track_init(&state);
    time_track_advance(&state, TIME_TRACK_SECONDS_PER_DAY - 40);
    time_track_handle(&state, TIME_TRACK_CMD_OK);
    time_track_advance(&state, TIME_TRACK_SECONDS_PER_DAY + 20);

    uint32_t today[TIME_TRACK_LABEL_COUNT];
    uint32_t yesterday[TIME_TRACK_LABEL_COUNT];
    time_track_view_totals(&state, TIME_TRACK_DAY_TODAY, today);
    time_track_view_totals(&state, TIME_TRACK_DAY_YESTERDAY, yesterday);
    assert(yesterday[TIME_TRACK_LABEL_INTERNSHIP] == 40);
    assert(today[TIME_TRACK_LABEL_INTERNSHIP] == 20);

    time_track_handle(&state, TIME_TRACK_CMD_OK);
    assert(state.yesterday[TIME_TRACK_LABEL_INTERNSHIP] == 40);
    assert(state.today[TIME_TRACK_LABEL_INTERNSHIP] == 20);
}

static void test_formats(void)
{
    char buf[16];
    time_track_format_hms(3723, buf, sizeof(buf));
    assert(strcmp(buf, "01:02:03") == 0);
    time_track_format_hm(3723, buf, sizeof(buf));
    assert(strcmp(buf, "1h 2m") == 0);
}

int main(void)
{
    test_start_stop_creates_record();
    test_cannot_change_label_while_running();
    test_today_bars_and_totals();
    test_switch_yesterday_view();
    test_power_loss_recovers_open_session();
    test_midnight_split_and_day_roll();
    test_formats();
    return 0;
}
