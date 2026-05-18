#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

#include <freertos/FreeRTOS.h> // IWYU pragma: keep

#include "freertos/idf_additions.h"
#include "hal/gpio_types.h"
#include "pb.h"
#include "portmacro.h"
#include "usb/cdc_acm_host.h"
#include "usb/cdc_acm_host_ops.h"
#include "usb/cdc_host_types.h"
#include "usb/usb_host.h"
#include "usb/usb_types_cdc.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <pb_decode.h>
#include <smak.pb.h>

#include <smak_defines.h>
#include <smak_http_interface.h>
#include <smak_util.h>

extern void softap_main(void);

#define USB_HOST_PRIO (20)
#define TX_STRING ("smak test string")
#define TX_TIMEOUT_MS (1000)
#define MAX_CDC_DEVICES (5)
#define VID (0x303A)
#define APP_QUIT_PIN 0

static const char *TAG = "usb_cdc_acm_host";

static cdc_acm_dev_hdl_t cdc_devices[MAX_CDC_DEVICES] = { 0 };

static QueueHandle_t send_queue = { 0 };
static QueueHandle_t app_queue  = { 0 };

typedef struct {
    enum {
        APP_QUIT,
        APP_DEVICE_CONNECTED,
        APP_DEVICE_DISCONNECTED,
    } id;
    union {
        struct {
            uint16_t vid;
            uint16_t pid;
        } new_dev;
        int32_t device_slot;
    } data;
} app_message_t;

static inline int get_free_slot(void)
{
    for (size_t i = 0; i < MAX_CDC_DEVICES; i++) {
        if (cdc_devices[i] == NULL) {
            return i;
        }
    }
    return -1;
}

[[maybe_unused]] uint8_t json_buffer[512]        = { 0 };
[[maybe_unused]] static size_t json_buffer_index = { 0 };

static bool handle_rx(const uint8_t *data, size_t data_len, void *arg)
{

#ifdef SMAK_JSON
    int32_t slot = (int)arg;
    ESP_LOGI(TAG, "Data received at slot %d", slot);
    ESP_LOG_BUFFER_HEXDUMP(TAG, data, data_len, ESP_LOG_INFO);

    if ((sizeof(json_buffer) - json_buffer_index) < data_len) {
        ESP_LOGE(TAG, "Data doesn't fit in buffer");
        return false;
    }

    memcpy(&json_buffer[json_buffer_index], data, data_len);

    json_buffer_index += data_len;
    if (json_buffer[json_buffer_index - 1] == '\n' && json_buffer[json_buffer_index - 2] == '\r') {
        ESP_LOGI(TAG, "Processed json obj");
        json_buffer[json_buffer_index] = '\0';

        json_buffer_index = 0;

        /** @todo Send til en reserve-kø-agtig ting */
        xQueueSend(send_queue, &json_buffer, 0);
    }
    return true;
#else

    // [[maybe_unused]] int32_t slot = (int)arg;
    // if (sizeof(json_buffer) < data_len) {
    //     ESP_LOGE(TAG, "No space in buffer");
    //     return false;
    // }
    // // ESP_LOG_BUFFER_HEXDUMP(TAG, data, data_len, ESP_LOG_INFO);

    int32_t slot = (int)arg;
    ESP_LOGI(TAG, "Data received at slot %d", slot);
    ESP_LOG_BUFFER_HEXDUMP(TAG, data, data_len, ESP_LOG_INFO);

    if (data_len < SMAK_CHESS_MOVE_SIZE) {

        struct smak_pb_ctx ctx = {
            .msg_size = data_len
        };

        memcpy(ctx.buffer, data, data_len);

        xQueueSend(send_queue, &ctx, 0);

        return true;
    } else {
        return false;
    }
#endif
}

static void handle_event(const cdc_acm_host_dev_event_data_t *event, void *user_ctx)
{
    switch (event->type) {
    case CDC_ACM_HOST_ERROR:
        ESP_LOGE(TAG, "CDC ACM error (%d)", event->data.error);
        break;
    case CDC_ACM_HOST_DEVICE_DISCONNECTED:
        if (app_queue) {
            app_message_t msg = {
                .id               = APP_DEVICE_DISCONNECTED,
                .data.device_slot = (int)(intptr_t)user_ctx
            };
            xQueueSend(app_queue, &msg, 0);
        } else {
            ESP_ERROR_CHECK(cdc_acm_host_close(event->data.cdc_hdl));
        }
        break;
    case CDC_ACM_HOST_SERIAL_STATE:
        ESP_LOGI(TAG, "Serial state notification 0x%04X", event->data.serial_state.val);
        break;
    default:
        ESP_LOGW(TAG, "Unknown or unsupported CDC event");
        break;
    }
}

static void new_dev_cb(usb_device_handle_t usb_dev)
{
    const usb_device_desc_t *device_desc;
    esp_err_t err = usb_host_get_device_descriptor(usb_dev, &device_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get device descriptor: %s", esp_err_to_name(err));
        return;
    }

    uint16_t vid = device_desc->idVendor;
    uint16_t pid = device_desc->idProduct;
    ESP_LOGI(TAG, "CDC device connected. VID = 0x%04X PID = 0x%04X", vid, pid);

    if (app_queue) {
        app_message_t msg = {
            .id               = APP_DEVICE_CONNECTED,
            .data.new_dev.pid = pid,
            .data.new_dev.vid = vid,
        };
        xQueueSend(app_queue, &msg, 0);
    }
}

static cdc_acm_dev_hdl_t cdc_open(uint16_t vid, uint16_t pid, const cdc_acm_host_device_config_t *dev_cfg)
{
    cdc_acm_dev_hdl_t cdc_dev = { NULL };
    esp_err_t err             = { 0 };

    err = cdc_acm_host_open(vid, pid, 0, dev_cfg, &cdc_dev);

    if (err == ESP_OK) {
        return cdc_dev;
    }

    ESP_LOGE(TAG, "Failed to open USB device with VID = 0x%04X PID = 0x%04X", vid, pid);
    return NULL;
}

static void free_cdc_device(int slot)
{
    if (slot < 0 || slot >= MAX_CDC_DEVICES || cdc_devices[slot] == NULL) {
        return;
    }
    ESP_LOGI(TAG, "Closing CDC device in slot %d", slot);
    cdc_acm_host_close(cdc_devices[slot]);
    cdc_devices[slot] = NULL;
}

static void free_all_cdc_devices(void)
{
    for (size_t i = 0; i < MAX_CDC_DEVICES; i++) {
        free_cdc_device(i);
    }
}

static void cdc_test(int slot)
{
    return;
    cdc_acm_dev_hdl_t cdc_dev = cdc_devices[slot];
    ESP_LOGI(TAG, "CDC device opened in slot %d", slot);
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "Data transfer test");
    ESP_ERROR_CHECK(cdc_acm_host_data_tx_blocking(cdc_dev, (const uint8_t *)TX_STRING, strlen(TX_STRING), TX_TIMEOUT_MS));

    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "Control line state cmd test");
    esp_err_t err = cdc_acm_host_set_control_line_state(cdc_dev, false, false);
    if (err == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGW(TAG, "Control line set state not supported");
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Control line set state failed");
    } else {
        vTaskDelay(pdMS_TO_TICKS(20));
        cdc_acm_host_set_control_line_state(cdc_dev, false, true);
        ESP_LOGI(TAG, "dtr = false, rts = true");
        vTaskDelay(pdMS_TO_TICKS(20));
        cdc_acm_host_set_control_line_state(cdc_dev, false, false);
    }

    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "Line coding cmd test");
    cdc_acm_line_coding_t line_coding = { 0 };

    err = cdc_acm_host_line_coding_get(cdc_dev, &line_coding);
    if (err == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGW(TAG, "Line coding get cmd not supported");
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Line coding get cmd failed");
    } else {
        ESP_LOGI(TAG, "Line coding is: Rate = %u, Stop bits = %u, Parity = %u, Data bits = %u",
                 line_coding.dwDTERate,
                 line_coding.bCharFormat,
                 line_coding.bParityType,
                 line_coding.bDataBits);

#define LINE_CODING_STOPBITS_1 0

        ESP_LOGI(TAG, "Setting line coding");
        line_coding.dwDTERate   = 115200;
        line_coding.bDataBits   = 8;
        line_coding.bParityType = 0;
        line_coding.bCharFormat = LINE_CODING_STOPBITS_1;

        err = cdc_acm_host_line_coding_set(cdc_dev, &line_coding);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Line coding set cmd failed");
        } else {
            ESP_LOGI(TAG, "Line coding is: Rate = %u, Stop bits = %u, Parity = %u, Data bits = %u",
                     line_coding.dwDTERate,
                     line_coding.bCharFormat,
                     line_coding.bParityType,
                     line_coding.bDataBits);
        }
    }
    ESP_LOGI(TAG, "Line command test done");
}

static void gpio_cb(void *arg)
{
    (void)arg;
    BaseType_t xTaskWoken = pdFALSE;

    app_message_t msg = {
        .id = APP_QUIT
    };
    if (app_queue) {
        xQueueSendToFrontFromISR(app_queue, &msg, &xTaskWoken);
    }
    if (xTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

static void usb_lib_task(void *arg)
{
    const static char *USB_TASK_TAG = __func__;
    ESP_LOGI(USB_TASK_TAG, "USB task running");
    const usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags     = ESP_INTR_FLAG_LOWMED,
    };
    ESP_LOGI(USB_TASK_TAG, "Installing USB host driver");
    ESP_ERROR_CHECK(usb_host_install(&host_config));

    const cdc_acm_host_driver_config_t driver_config = {
        .driver_task_stack_size = 4096,
        .driver_task_priority   = USB_HOST_PRIO + 1,
        .xCoreID                = 0,
        .new_dev_cb             = new_dev_cb
    };
    ESP_LOGI(USB_TASK_TAG, "Installing CDC-ACM driver");
    ESP_ERROR_CHECK(cdc_acm_host_install(&driver_config));

    xTaskNotifyGive(arg);

    bool has_clients = true;
    while (1) {
        uint32_t event_flags = { 0 };
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            has_clients = false;
            if (usb_host_device_free_all() == ESP_OK) {
                break;
            }
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            if (!has_clients) {
                break;
            }
        }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_ERROR_CHECK(usb_host_uninstall());
    ESP_LOGI(USB_TASK_TAG, "USB host task done");
    vTaskDelete(NULL);
}

static bool running = true;
void read_queue_task(void *arg)
{
    while (1) {
        ESP_LOGI(__func__, "Progressing in queue read");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if (ulTaskNotifyTake(pdTRUE, 0)) {
            break;
        }
        struct smak_pb_ctx ctx = { 0 };
        xQueueReceive(send_queue, &ctx, portMAX_DELAY);

        smak_chess_move_t decoded = { 0 };

        pb_istream_t istream = pb_istream_from_buffer((pb_byte_t *)ctx.buffer, ctx.msg_size);
        pb_decode(&istream, SMAK_CHESS_MOVE_FIELDS, &decoded);

        SMAK_LOGI("id = %llu", decoded.id);

        SMAK_LOGI("id = %llu, ply = %u, from = %u, to = %u, piece = %c, captured = %c, move_type = %s",
                  decoded.id, decoded.ply, decoded.from, decoded.to, decoded.piece.bytes[0], decoded.captured.bytes[0], move_type_strings[decoded.move_type]);

        decoded.id = 3;
        smak_http_post_move(70, &decoded, 0);
    }
    SMAK_LOGI("Task done");
    vTaskDelete(NULL);
}

TaskFunction_t get_smak_token_task(void);

extern void smak_ota_main(void);

void app_main(void)
{
    softap_main();
    SMAK_LOGI("STACK SIZE: %d", CONFIG_MAIN_TASK_STACK_SIZE);
    vTaskDelay(5000 / portTICK_PERIOD_MS); // wifi laver finurlige ting hvis vi ikke venter lidt (måske)

    smak_ota_main();

    vTaskDelay(portMAX_DELAY);

    test_smak_json_print();
    smak_http_auth_token_get("smakauth.head9x.dk");
    // call_post();

    app_queue = xQueueCreate(16, sizeof(app_message_t));
    assert(app_queue);
    send_queue = xQueueCreate(8, sizeof(struct smak_pb_ctx));
    assert(send_queue);
    BaseType_t task_created = xTaskCreatePinnedToCore(usb_lib_task,
                                                      "usb_lib",
                                                      4096,
                                                      xTaskGetCurrentTaskHandle(),
                                                      USB_HOST_PRIO,
                                                      NULL, 0);

    assert(task_created == pdTRUE);
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    TaskHandle_t read_queue_handle = { 0 };

    task_created = xTaskCreate(read_queue_task,
                               "read_queue",
                               4096,
                               xTaskGetCurrentTaskHandle(),
                               USB_HOST_PRIO + 2,
                               &read_queue_handle);

    // TaskHandle_t tok_task_handle = { 0 };

    // task_created = xTaskCreate(get_smak_token_task(), "token_get", 32784, xTaskGetCurrentTaskHandle(), 22, &tok_task_handle);

    const gpio_config_t input_pin = {
        .pin_bit_mask = BIT64(APP_QUIT_PIN),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,
    };

    ESP_ERROR_CHECK(gpio_config(&input_pin));
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_LOWMED));
    ESP_ERROR_CHECK(gpio_isr_handler_add(APP_QUIT_PIN, gpio_cb, NULL));

    cdc_acm_host_device_config_t dev_config = {
        .connection_timeout_ms = 0,
        .out_buffer_size       = 512,
        .in_buffer_size        = 0,
        .user_arg              = NULL,
        .event_cb              = handle_event,
        .data_cb               = handle_rx,
    };

    ESP_LOGI(__func__, "Waiting for CDC devices");

    while (running) {
        app_message_t msg = { 0 };

        xQueueReceive(app_queue, &msg, portMAX_DELAY);

        switch (msg.id) {
        case APP_DEVICE_CONNECTED: {
            int slot = get_free_slot();
            if (slot < 0) {
                ESP_LOGW(__func__, "No free slots left.");
                continue;
            }
            dev_config.user_arg       = (void *)(intptr_t)slot;
            cdc_acm_dev_hdl_t cdc_dev = cdc_open(msg.data.new_dev.vid, msg.data.new_dev.pid, &dev_config);
            if (cdc_dev == NULL) {
                continue;
            }

            cdc_devices[slot] = cdc_dev;
            cdc_test(slot);
            break;
        }

        case APP_DEVICE_DISCONNECTED: {
            ESP_LOGI(__func__, "CDC device disconnected from slot %d", msg.data.device_slot);
            free_cdc_device(msg.data.device_slot);
            break;
        }
        case APP_QUIT: {
            ESP_LOGI(__func__, "Exiting");
            free_all_cdc_devices();
            ESP_ERROR_CHECK(cdc_acm_host_uninstall());
            running = false;
            break;
        }
        default:
            ESP_LOGW(__func__, "Unknown message ID: %d", msg.id);
        }
    }

    ESP_LOGI(__func__, "Exit");
    xTaskNotifyGive(read_queue_handle);
    gpio_isr_handler_remove(APP_QUIT_PIN);
    gpio_uninstall_isr_service();
    vQueueDelete(app_queue);
    vQueueDelete(send_queue);
}
