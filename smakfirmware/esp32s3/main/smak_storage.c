#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <smak_defines.h>
#include <stdint.h>

static uint64_t current_id      = { 0 };
static bool id_is_stored_in_nvs = false;

int smak_storage_init(void)
{
    id_is_stored_in_nvs = false;
    nvs_handle_t handle = { 0 };

    esp_err_t err = nvs_open("smak_games", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        SMAK_LOGE("nvs_open() failed");
        return -1;
    }

    uint64_t id = { 0 };

    err = nvs_get_u64(handle, SMAK_GAME_ID_NVS_KEY, &id);
    if (err != ESP_OK) {
        SMAK_LOGW("No ID stored in NVS");
        SMAK_LOGI("Waiting for a new game to be posted to update ID instead");
    }

    nvs_close(handle);

    return 0;
}

uint64_t smak_gameid_current_get(void)
{

    uint64_t id         = { 0 };
    nvs_handle_t handle = { 0 };

    esp_err_t err = nvs_open("smak_games", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        SMAK_LOGE("nvs_open() failed");
        return 0;
    }

    err = nvs_get_u64(handle, SMAK_GAME_ID_NVS_KEY, &id);
    if (err != ESP_OK) {
        SMAK_LOGW("No ID stored in NVS");
        SMAK_LOGI("Waiting for a new game to be posted to update ID instead");
    }

    nvs_close(handle);

    return id;

    // return current_id;
}

int smak_gameid_store(uint64_t id)
{

    if (id == current_id) {
        return 0;
    }

    nvs_handle_t handle = { 0 };
    esp_err_t err       = nvs_open("smak_games", NVS_READWRITE, &handle);

    if (err != ESP_OK) {
        SMAK_LOGE("nvs_open() failed");
        return -1;
    }

    err = nvs_set_u64(handle, SMAK_GAME_ID_NVS_KEY, id);
    if (err != ESP_OK) {
        SMAK_LOGE("Failed to store game ID in NVS");
        return -2;
    }
    nvs_commit(handle);
    nvs_close(handle);

    current_id = id;

    return 0;
}
