#include "time_track_store.h"

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

static const char *TAG = "time_track_store";
static const char *NS = "timetrack";
static const char *KEY = "state";

static bool s_ready;

esp_err_t time_track_store_init(void)
{
    if (s_ready) {
        return ESP_OK;
    }

    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %s; records will not persist", esp_err_to_name(err));
        return err;
    }
    s_ready = true;
    return ESP_OK;
}

esp_err_t time_track_store_load(time_track_state_t *state)
{
    if (!state) {
        return ESP_ERR_INVALID_ARG;
    }
    time_track_init(state);
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NS, NVS_READONLY, &handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(err));
        return err;
    }

    uint8_t blob[TIME_TRACK_STORE_SIZE];
    size_t size = sizeof(blob);
    err = nvs_get_blob(handle, KEY, blob, &size);
    nvs_close(handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS read failed: %s", esp_err_to_name(err));
        return err;
    }
    if (!time_track_import(state, blob, size)) {
        ESP_LOGW(TAG, "stored state rejected; starting empty");
        time_track_init(state);
        return ESP_ERR_INVALID_CRC;
    }
    return ESP_OK;
}

esp_err_t time_track_store_save(const time_track_state_t *state)
{
    if (!state) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t blob[TIME_TRACK_STORE_SIZE];
    if (time_track_export(state, blob, sizeof(blob)) != TIME_TRACK_STORE_SIZE) {
        return ESP_ERR_INVALID_SIZE;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NS, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(err));
        return err;
    }
    err = nvs_set_blob(handle, KEY, blob, sizeof(blob));
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS save failed: %s", esp_err_to_name(err));
    }
    return err;
}
