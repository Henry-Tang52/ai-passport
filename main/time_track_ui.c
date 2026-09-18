#include "time_track_ui.h"
#include "time_track_text.h"

#include "lvgl.h"

#include <stdio.h>
#include <string.h>

#define TT_BG        0x0B1220
#define TT_SURFACE   0x12202C
#define TT_TEAL      0x2DD4BF
#define TT_TEAL_LINE 0x115E59
#define TT_TEXT      0xE8F1F2
#define TT_MUTED     0x8AA3B0
#define TT_BAR_BG    0x1E293B
#define TT_LOW_BATT  0xF87171
#define TT_BREATH_MIN 120
#define TT_BREATH_MAX 255

#define TT_BAR_MAX_PX 80

/* Soft, distinct fills for the six Today-page proportion bars. */
static const uint32_t TT_BAR_COLORS[TIME_TRACK_LABEL_COUNT] = {
    0x3AA89A,
    0x5A8FB5,
    0xC4A35A,
    0xC47C74,
    0x8B7EB5,
    0x5F9A78,
};

LV_FONT_DECLARE(time_track_font_12);
LV_FONT_DECLARE(time_track_font_16);
LV_FONT_DECLARE(time_track_font_20);

typedef struct {
    lv_obj_t *label;
    lv_obj_t *weight;
    lv_obj_t *duration;
    lv_obj_t *bar_fill;
} today_row_t;

static lv_obj_t *s_scr;
static lv_obj_t *s_battery;
static lv_obj_t *s_timer_page;
static lv_obj_t *s_today_page;
static lv_obj_t *s_timer_card;
static lv_obj_t *s_timer_label;
static lv_obj_t *s_timer_state;
static lv_obj_t *s_timer_elapsed;
static lv_obj_t *s_today_header;
static today_row_t s_rows[TIME_TRACK_LABEL_COUNT];
static bool s_timing_border;

static lv_obj_t *make_label(lv_obj_t *parent, uint32_t color, const lv_font_t *font)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
    return label;
}

static lv_obj_t *make_panel(lv_obj_t *parent, int x, int y, int w, int h)
{
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(panel, x, y);
    lv_obj_set_size(panel, w, h);
    lv_obj_set_style_bg_color(panel, lv_color_hex(TT_SURFACE), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(TT_TEAL_LINE), 0);
    lv_obj_set_style_border_width(panel, 2, 0);
    lv_obj_set_style_radius(panel, 14, 0);
    lv_obj_set_style_pad_all(panel, 0, 0);
    return panel;
}

static void timer_border_opa(void *obj, int32_t value)
{
    lv_obj_set_style_border_opa((lv_obj_t *)obj, (lv_opa_t)value, 0);
}

static void set_timer_card_timing(bool timing)
{
    if (!s_timer_card || timing == s_timing_border) {
        return;
    }

    s_timing_border = timing;
    lv_anim_delete(s_timer_card, timer_border_opa);
    if (!timing) {
        lv_obj_set_style_border_color(s_timer_card, lv_color_hex(TT_TEAL_LINE), 0);
        lv_obj_set_style_border_opa(s_timer_card, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(s_timer_card, 2, 0);
        return;
    }

    lv_obj_set_style_border_color(s_timer_card, lv_color_hex(TT_TEAL), 0);
    lv_obj_set_style_border_width(s_timer_card, 3, 0);
    lv_anim_t anim;
    lv_anim_init(&anim);
    lv_anim_set_var(&anim, s_timer_card);
    lv_anim_set_exec_cb(&anim, timer_border_opa);
    lv_anim_set_values(&anim, TT_BREATH_MIN, TT_BREATH_MAX);
    lv_anim_set_duration(&anim, 900);
    lv_anim_set_playback_duration(&anim, 900);
    lv_anim_set_repeat_count(&anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&anim, lv_anim_path_ease_in_out);
    lv_anim_start(&anim);
}

void time_track_ui_create(void)
{
    s_scr = lv_obj_create(NULL);
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_scr, lv_color_hex(TT_BG), 0);
    lv_obj_set_style_bg_opa(s_scr, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_scr, 0, 0);
    lv_obj_set_style_pad_all(s_scr, 0, 0);

    lv_obj_t *accent = lv_obj_create(s_scr);
    lv_obj_remove_flag(accent, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(accent, 16, 10);
    lv_obj_set_size(accent, 48, 4);
    lv_obj_set_style_bg_color(accent, lv_color_hex(TT_TEAL), 0);
    lv_obj_set_style_bg_opa(accent, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(accent, 0, 0);
    lv_obj_set_style_radius(accent, 2, 0);
    lv_obj_set_style_pad_all(accent, 0, 0);

    lv_obj_t *title = make_label(s_scr, TT_TEAL, &time_track_font_12);
    lv_label_set_text(title, TIME_TRACK_TEXT_TITLE);
    lv_obj_set_pos(title, 16, 22);

    s_battery = make_label(s_scr, TT_MUTED, &time_track_font_16);
    lv_label_set_text(s_battery, "");
    lv_obj_align(s_battery, LV_ALIGN_TOP_RIGHT, -16, 20);

    s_timer_page = lv_obj_create(s_scr);
    lv_obj_remove_flag(s_timer_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(s_timer_page, 0, 48);
    lv_obj_set_size(s_timer_page, 240, 272);
    lv_obj_set_style_bg_opa(s_timer_page, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_timer_page, 0, 0);
    lv_obj_set_style_pad_all(s_timer_page, 0, 0);

    s_timer_card = make_panel(s_timer_page, 16, 20, 208, 176);
    s_timer_label = make_label(s_timer_card, TT_TEAL, &time_track_font_16);
    lv_obj_align(s_timer_label, LV_ALIGN_TOP_MID, 0, 16);
    s_timer_state = make_label(s_timer_card, TT_MUTED, &time_track_font_16);
    lv_obj_align(s_timer_state, LV_ALIGN_TOP_MID, 0, 42);
    s_timer_elapsed = make_label(s_timer_card, TT_TEXT, &lv_font_montserrat_32);
    lv_obj_align(s_timer_elapsed, LV_ALIGN_TOP_MID, 0, 84);

    lv_obj_t *hint = make_label(s_timer_page, TT_MUTED, &time_track_font_16);
    lv_label_set_text(hint, "OK  /  OK long");
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -18);

    s_today_page = lv_obj_create(s_scr);
    lv_obj_remove_flag(s_today_page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(s_today_page, 0, 48);
    lv_obj_set_size(s_today_page, 240, 272);
    lv_obj_set_style_bg_opa(s_today_page, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_today_page, 0, 0);
    lv_obj_set_style_pad_all(s_today_page, 0, 0);
    lv_obj_add_flag(s_today_page, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *list = make_panel(s_today_page, 12, 8, 216, 248);
    s_today_header = make_label(list, TT_TEAL, &time_track_font_20);
    lv_obj_align(s_today_header, LV_ALIGN_TOP_LEFT, 12, 8);

    for (int i = 0; i < TIME_TRACK_LABEL_COUNT; i++) {
        const int y = 42 + i * 33;
        s_rows[i].label = make_label(list, TT_TEXT, &time_track_font_16);
        lv_label_set_text(s_rows[i].label, TIME_TRACK_LABEL_TEXT[i]);
        lv_obj_set_pos(s_rows[i].label, 12, y);

        /* 1px offset duplicate: cheap CJK faux-bold without a second weight. */
        s_rows[i].weight = make_label(list, TT_TEXT, &time_track_font_16);
        lv_label_set_text(s_rows[i].weight, TIME_TRACK_LABEL_TEXT[i]);
        lv_obj_set_pos(s_rows[i].weight, 13, y);
        lv_obj_add_flag(s_rows[i].weight, LV_OBJ_FLAG_HIDDEN);

        s_rows[i].duration = make_label(list, TT_MUTED, &time_track_font_16);
        lv_obj_set_pos(s_rows[i].duration, 50, y);

        lv_obj_t *track = lv_obj_create(list);
        lv_obj_remove_flag(track, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_pos(track, 124, y + 6);
        lv_obj_set_size(track, TT_BAR_MAX_PX, 10);
        lv_obj_set_style_bg_color(track, lv_color_hex(TT_BAR_BG), 0);
        lv_obj_set_style_bg_opa(track, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(track, 0, 0);
        lv_obj_set_style_radius(track, 5, 0);
        lv_obj_set_style_pad_all(track, 0, 0);

        s_rows[i].bar_fill = lv_obj_create(track);
        lv_obj_remove_flag(s_rows[i].bar_fill, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_pos(s_rows[i].bar_fill, 0, 0);
        lv_obj_set_size(s_rows[i].bar_fill, 0, 10);
        lv_obj_set_style_bg_color(s_rows[i].bar_fill, lv_color_hex(TT_BAR_COLORS[i]), 0);
        lv_obj_set_style_bg_opa(s_rows[i].bar_fill, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(s_rows[i].bar_fill, 0, 0);
        lv_obj_set_style_radius(s_rows[i].bar_fill, 5, 0);
        lv_obj_set_style_pad_all(s_rows[i].bar_fill, 0, 0);
    }

    s_timing_border = false;
    lv_screen_load(s_scr);
}

void time_track_ui_refresh(const time_track_state_t *state, int battery_soc)
{
    if (!s_scr || !state) {
        return;
    }

    if (battery_soc < 0) {
        lv_label_set_text(s_battery, "");
    } else {
        char soc_text[16];
        const int soc = battery_soc > 100 ? 100 : battery_soc;
        snprintf(soc_text, sizeof(soc_text), "%d%%", soc);
        lv_label_set_text(s_battery, soc_text);
        lv_obj_set_style_text_color(
            s_battery,
            lv_color_hex(battery_soc < 20 ? TT_LOW_BATT : TT_MUTED),
            LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    const bool today_page = state->page == TIME_TRACK_PAGE_TODAY;
    if (today_page) {
        lv_obj_add_flag(s_timer_page, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_today_page, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_today_page, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_timer_page, LV_OBJ_FLAG_HIDDEN);
    }

    lv_label_set_text(s_timer_label, TIME_TRACK_LABEL_TEXT[time_track_active_label(state)]);
    lv_label_set_text(s_timer_state,
                      state->session.running ? TIME_TRACK_TEXT_RUNNING : TIME_TRACK_TEXT_IDLE);
    lv_obj_set_style_text_color(
        s_timer_state,
        lv_color_hex(state->session.running ? TT_TEAL : TT_MUTED),
        LV_PART_MAIN | LV_STATE_DEFAULT);
    set_timer_card_timing(state->session.running && !today_page);

    char elapsed[20];
    time_track_format_hms(time_track_elapsed(state), elapsed, sizeof(elapsed));
    lv_label_set_text(s_timer_elapsed, elapsed);

    uint32_t values[TIME_TRACK_LABEL_COUNT];
    time_track_view_totals(state, state->day_view, values);
    const uint32_t total = time_track_total(values);
    char hm[24];
    time_track_format_hm(total, hm, sizeof(hm));
    lv_label_set_text_fmt(
        s_today_header, "%s%s",
        state->day_view == TIME_TRACK_DAY_YESTERDAY
            ? TIME_TRACK_TEXT_YESTERDAY_TOTAL
            : TIME_TRACK_TEXT_TODAY_TOTAL,
        hm);

    for (int i = 0; i < TIME_TRACK_LABEL_COUNT; i++) {
        time_track_format_hm(values[i], hm, sizeof(hm));
        lv_label_set_text(s_rows[i].duration, hm);
        const uint16_t width = time_track_bar_px(values[i], total, TT_BAR_MAX_PX);
        lv_obj_set_size(s_rows[i].bar_fill, width, 10);
        if (values[i] > 0) {
            lv_obj_remove_flag(s_rows[i].weight, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_rows[i].weight, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void time_track_ui_destroy(void)
{
    if (!s_scr) {
        return;
    }
    if (s_timer_card) {
        lv_anim_delete(s_timer_card, timer_border_opa);
    }
    lv_obj_delete(s_scr);
    s_scr = NULL;
    s_battery = NULL;
    s_timer_page = NULL;
    s_today_page = NULL;
    s_timer_card = NULL;
    s_timer_label = NULL;
    s_timer_state = NULL;
    s_timer_elapsed = NULL;
    s_today_header = NULL;
    s_timing_border = false;
    memset(s_rows, 0, sizeof(s_rows));
}
