#pragma once

/* Fixed Simplified Chinese UI inventory. Keep in sync with tools/gen_time_track_font.py.
 * 12px covers the title, 16px covers the full set, 20px covers the day-total line. */

#define TIME_TRACK_TEXT_TITLE "用时记账"

#define TIME_TRACK_TEXT_IDLE "未在记"
#define TIME_TRACK_TEXT_RUNNING "计时中"

#define TIME_TRACK_TEXT_TODAY_TOTAL "今日共 "
#define TIME_TRACK_TEXT_YESTERDAY_TOTAL "昨天共 "

static const char *const TIME_TRACK_LABEL_TEXT[6] = {
    "实习",
    "投递",
    "生活",
    "面试",
    "学校",
    "运动",
};

#define TIME_TRACK_FONT_SYMBOLS "用时记账实习投递生活面试学校运动未在记计中今日共昨天"
