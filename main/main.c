// 用时记账 v0: offline efficiency timer. Custom screens only; no demo menu.
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "time_track_store.h"
#include "time_track_ui.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "time_track";

#define INPUT_QUEUE_DEPTH 8
#define HEARTBEAT_SEC 10u
#define UI_PERIOD_MS 500
#define BATTERY_PERIOD_MS 5000
#define IDLE_DIM_MS 30000

typedef struct {
    bsp_btn_t btn;
    bsp_btn_ev_t event;
} input_event_t;

static time_track_state_t s_state;
static QueueHandle_t s_input_queue;
static TaskHandle_t s_input_task;
static SemaphoreHandle_t s_lock;
static volatile bool s_input_ready;
static uint32_t s_base_now;
static int64_t s_mono_us;
static uint32_t s_last_persist_now;
static int64_t s_last_input_us;
static int s_soc = -1;
static bool s_dimmed;

static uint32_t current_now(void)
{
    int64_t delta_sec = (esp_timer_get_time() - s_mono_us) / 1000000LL;
    if (delta_sec < 0) {
        delta_sec = 0;
    }
    return s_base_now + (uint32_t)delta_sec;
}

static void persist_state(void)
{
    if (time_track_store_save(&s_state) == ESP_OK) {
        s_last_persist_now = s_state.now_sec;
    }
}

static void refresh_ui(void)
{
    if (!bsp_lvgl_lock(200)) {
        return;
    }
    time_track_ui_refresh(&s_state, s_soc);
    bsp_lvgl_unlock();
}

static void apply_now(bool force_save)
{
    const bool rolled = time_track_advance(&s_state, current_now());
    const bool heartbeat = s_state.session.running
        && s_state.now_sec >= s_last_persist_now + HEARTBEAT_SEC;
    if (force_save || rolled || heartbeat) {
        persist_state();
    }
}

static void set_backlight_for_idle(void)
{
    const int64_t idle_us = esp_timer_get_time() - s_last_input_us;
    if (idle_us >= (int64_t)IDLE_DIM_MS * 1000 && !s_dimmed) {
        bsp_display_backlight(8);
        s_dimmed = true;
    }
}

static void wake_backlight(void)
{
    if (s_dimmed) {
        bsp_display_backlight(100);
        s_dimmed = false;
    }
    s_last_input_us = esp_timer_get_time();
}

static void handle_input(const input_event_t *input)
{
    time_track_cmd_t cmd;
    if (input->event == BSP_BTN_LONG && input->btn == BSP_BTN_OK) {
        cmd = TIME_TRACK_CMD_OK_LONG;
    } else if (input->event == BSP_BTN_CLICK && input->btn == BSP_BTN_UP) {
        cmd = TIME_TRACK_CMD_UP;
    } else if (input->event == BSP_BTN_CLICK && input->btn == BSP_BTN_DOWN) {
        cmd = TIME_TRACK_CMD_DOWN;
    } else if (input->event == BSP_BTN_CLICK && input->btn == BSP_BTN_OK) {
        cmd = TIME_TRACK_CMD_OK;
    } else {
        return;
    }

    xSemaphoreTake(s_lock, portMAX_DELAY);
    apply_now(false);
    time_track_handle(&s_state, cmd);
    persist_state();
    refresh_ui();
    xSemaphoreGive(s_lock);
}

static void input_task(void *arg)
{
    (void)arg;
    input_event_t input;
    int64_t last_battery_us = 0;
    for (;;) {
        if (xQueueReceive(s_input_queue, &input, pdMS_TO_TICKS(UI_PERIOD_MS)) == pdTRUE) {
            wake_backlight();
            handle_input(&input);
        }

        xSemaphoreTake(s_lock, portMAX_DELAY);
        apply_now(false);
        refresh_ui();
        xSemaphoreGive(s_lock);
        set_backlight_for_idle();

        const int64_t now_us = esp_timer_get_time();
        if (now_us - last_battery_us >= (int64_t)BATTERY_PERIOD_MS * 1000) {
            s_soc = bsp_battery_soc();
            last_battery_us = now_us;
        }
    }
}

static void on_key(bsp_btn_t btn, bsp_btn_ev_t ev, void *user)
{
    (void)user;
    if (!s_input_ready || !s_input_queue) {
        return;
    }
    const input_event_t input = { .btn = btn, .event = ev };
    (void)xQueueSend(s_input_queue, &input, 0);
}

void app_main(void)
{
    ESP_LOGI(TAG, "用时记账 v0 start");

    s_lock = xSemaphoreCreateMutex();
    s_input_queue = xQueueCreate(INPUT_QUEUE_DEPTH, sizeof(input_event_t));
    if (!s_lock || !s_input_queue) {
        ESP_LOGE(TAG, "cannot create input primitives");
        return;
    }

    time_track_init(&s_state);
    if (time_track_store_init() == ESP_OK) {
        (void)time_track_store_load(&s_state);
        time_track_recover_open(&s_state);
        persist_state();
    }

    s_base_now = s_state.now_sec;
    s_mono_us = esp_timer_get_time();
    s_last_persist_now = s_state.now_sec;
    s_last_input_us = s_mono_us;

    bsp_i2c_init();
    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "display/LVGL init failed");
        return;
    }
    bsp_display_backlight(100);

    if (bsp_battery_init() == ESP_OK) {
        s_soc = bsp_battery_soc();
    }

    if (bsp_lvgl_lock(1000)) {
        time_track_ui_create();
        time_track_ui_refresh(&s_state, s_soc);
        bsp_lvgl_unlock();
    }

    if (xTaskCreate(input_task, "tt_input", 4096, NULL, 5, &s_input_task) != pdPASS) {
        ESP_LOGE(TAG, "cannot create input task");
        return;
    }
    if (bsp_button_init(on_key, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "button init failed");
        return;
    }
    s_input_ready = true;
    ESP_LOGI(TAG, "ready");
}
