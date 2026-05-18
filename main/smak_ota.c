#include "esp_app_desc.h"
#include "esp_crt_bundle.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "freertos/idf_additions.h"
#include "nvs_flash.h"
#include "portmacro.h"
#include "smak_defines.h"
#include <assert.h>
#include <sdkconfig.h>
#include <smak_ota.h>
#include <stdint.h>
#include <string.h>
#include <sys/unistd.h>

void smak_ota_event_handler_impl(void *unused_arg, esp_event_base_t evt_base, esp_https_ota_event_t evt_id, void *evt_data)
{

    ARG_UNUSED(unused_arg);

    switch (evt_id) {

    case ESP_HTTPS_OTA_START:
    case ESP_HTTPS_OTA_CONNECTED:
    case ESP_HTTPS_OTA_GET_IMG_DESC:
    case ESP_HTTPS_OTA_VERIFY_CHIP_ID:
    case ESP_HTTPS_OTA_VERIFY_CHIP_REVISION:
    case ESP_HTTPS_OTA_DECRYPT_CB:
    case ESP_HTTPS_OTA_WRITE_FLASH:
    case ESP_HTTPS_OTA_UPDATE_BOOT_PARTITION:
    case ESP_HTTPS_OTA_FINISH:
    case ESP_HTTPS_OTA_ABORT:
        break;
    }
}

static_assert(sizeof(esp_https_ota_event_t) == sizeof(int32_t));
#define smak_ota_event_handler (esp_event_handler_t) smak_ota_event_handler_impl

[[nodiscard("Return value must be checked to ensure the downloaded image header is valid")]]
esp_err_t smak_ota_image_header_validate(esp_app_desc_t *app_info)
{
#define MEMCMP_RESULT_EQUAL 0
    if (!app_info) {
        return -1;
    }

    [[maybe_unused]] esp_err_t err = { 0 };

    const esp_partition_t *running_part = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info     = { 0 };

    err = esp_ota_get_partition_description(running_part, &running_app_info);

    bool same_as_current = { memcmp(app_info->version,
                                    running_app_info.version, sizeof(app_info->version))
                             == MEMCMP_RESULT_EQUAL };

    if (same_as_current) {
        SMAK_LOGW("New firmware image has same version as current");
        SMAK_LOGW("Not updating");
        return ESP_ERR_INVALID_VERSION;
    }

    return ESP_OK;

#undef MEMCMP_RESULT_EQUAL
}

void smak_ota_task(void *arg)
{
    SMAK_LOGI("OTA update start");

    esp_err_t err            = { 0 };
    esp_err_t ota_finish_err = { 0 };

    esp_http_client_config_t c_cfg = {
        .url                   = CONFIG_SMAK_FWUP_URL,
        .crt_bundle_attach     = esp_crt_bundle_attach,
        .timeout_ms            = CONFIG_SMAK_OTA_TIMEOUT,
        .keep_alive_enable     = true,
        .buffer_size           = 8192,
        .buffer_size_tx        = 4096,
        .max_redirection_count = 5
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config = &c_cfg,
    };

    esp_https_ota_handle_t ota = { 0 };

    err = esp_https_ota_begin(&ota_cfg, &ota);
    if (err != ESP_OK) {
        SMAK_LOGE("esp_https_ota_begin() returned %d", err);
        SMAK_LOGE("aborting...");
        goto ota_out;
    }

    esp_app_desc_t app_desc = { 0 };

    err = esp_https_ota_get_img_desc(ota, &app_desc);
    if (err != ESP_OK) {
        SMAK_LOGE("esp_https_ota_get_img_desc() returned %d", err);
        SMAK_LOGE("aborting...");
        goto ota_abort_and_out;
    }

    err = smak_ota_image_header_validate(&app_desc);
    if (err != ESP_OK) {
        SMAK_LOGE("Image header failed verification");
        goto ota_abort_and_out;
    }

    while (1) {
        err = esp_https_ota_perform(ota);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }
        [[maybe_unused]] size_t bytes_received = esp_https_ota_get_image_len_read(ota);
    }

    bool all_data_received = esp_https_ota_is_complete_data_received(ota);
    if (!all_data_received) {
        SMAK_LOGE("OTA update did not receive all data");
    }
    ota_finish_err = esp_https_ota_finish(ota);
    if (err == ESP_OK && ota_finish_err == ESP_OK) {
        SMAK_LOGI("OTA update done. Rebooting now");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        esp_restart();
    } else {
        SMAK_LOGE("OTA firmware upgrade failed (0x%04X)", ota_finish_err);
        TASK_DELETE_SELF();
    }

ota_abort_and_out:
    esp_https_ota_abort(ota);
ota_out:
    TASK_DELETE_SELF();
}

#ifdef CONFIG_SMAK_OTA_TASK_STACKSIZE
#define SMAK_OTA_TASK_STACK_SIZE CONFIG_SMAK_OTA_TASK_STACKSIZE
#else
#define SMAK_OTA_TASK_STACK_SIZE 8 * 1024
#endif

#define SMAK_OTA_TASK_PRIO 5
void smak_ota_main(void)
{
    esp_err_t err = { 0 };

    /// @todo Handle failed init and/or check if \p nvs_flash_init() already has been called
    err = nvs_flash_init();

    err = esp_event_loop_create_default();

    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        /// exit and die
        return;
    }

    err = esp_event_handler_register(ESP_HTTPS_OTA_EVENT, ESP_EVENT_ANY_ID, smak_ota_event_handler, NULL);

    xTaskCreate(smak_ota_task, "smak_ota_task", SMAK_OTA_TASK_STACK_SIZE, NULL, SMAK_OTA_TASK_PRIO, NULL);
}
