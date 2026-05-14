
#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif_types.h"
#include "esp_wifi.h"
#include "freertos/idf_additions.h"
#include "network_provisioning/scheme_softap.h"
#include "nvs_flash.h"
#include "qrcode.h"
#include <stdint.h>
const char *TAG = "smak_softap_wifi_prov";

#ifndef SMAK_PROD
#define SEC2_USERNAME "smakuser"
#define SEC2_PWD "smakpwd"
#endif

static const char sec2_salt[] = {
    0x52, 0xdd, 0xfa, 0xa9, 0x63, 0x3b, 0xde, 0x54, 0x96, 0x59, 0x34, 0x34, 0xa1, 0x03, 0xe8, 0x2e
};

static const char sec2_verifier[] = {
    0xc6, 0x78, 0xfa, 0xcc, 0x17, 0x62, 0x93, 0x3d, 0xa0, 0xd7, 0x54, 0x6f, 0xb9, 0x2d, 0xb2, 0xd2,
    0x3c, 0x37, 0xdd, 0x01, 0x3e, 0x94, 0x3c, 0x62, 0xad, 0x75, 0x1d, 0xe1, 0xbb, 0xb2, 0x9f, 0xb7,
    0xb6, 0xb5, 0x28, 0x3b, 0x63, 0x5c, 0xeb, 0x07, 0xdc, 0x5c, 0xd1, 0xfc, 0x6f, 0x6b, 0x33, 0xcc,
    0xc4, 0xbf, 0x86, 0xb4, 0x67, 0x8a, 0x87, 0xfc, 0x45, 0xb6, 0xaa, 0xdc, 0x29, 0x22, 0x6f, 0x3e,
    0x2a, 0xf8, 0xad, 0x77, 0x24, 0x2c, 0x1d, 0x74, 0xdb, 0x14, 0x40, 0x59, 0xc9, 0xf3, 0x19, 0xa3,
    0x5f, 0x74, 0x70, 0x23, 0x09, 0x6c, 0x30, 0x2e, 0x64, 0x04, 0xf1, 0xf6, 0xa1, 0x50, 0x46, 0x24,
    0xb5, 0x7f, 0x38, 0x6e, 0x1f, 0x7a, 0x42, 0x83, 0xd1, 0x73, 0x4d, 0x2b, 0x2a, 0x29, 0x48, 0xe9,
    0x13, 0x35, 0xb1, 0x00, 0x1b, 0x9a, 0xe4, 0xdb, 0x0c, 0x8e, 0x0f, 0x8b, 0x1a, 0xdd, 0xa9, 0x64,
    0x73, 0x5e, 0xa3, 0x78, 0x08, 0x01, 0xfb, 0xe4, 0x45, 0xb6, 0xb9, 0x9a, 0x91, 0x46, 0xe8, 0x1c,
    0x2a, 0x9a, 0x6e, 0xf8, 0xcd, 0xb4, 0x0b, 0x4a, 0xaa, 0xeb, 0x25, 0x64, 0x83, 0x06, 0xfe, 0x0b,
    0xd8, 0x61, 0x45, 0x8b, 0x26, 0xb6, 0x4a, 0x3a, 0x0c, 0x1c, 0x80, 0xc8, 0xb6, 0x03, 0x4c, 0x8e,
    0x61, 0x54, 0x0c, 0x37, 0x31, 0xe2, 0xec, 0x85, 0x77, 0x91, 0x9e, 0x98, 0x2f, 0xdf, 0xe8, 0xdf,
    0x69, 0x75, 0xce, 0xfd, 0x32, 0x53, 0x48, 0x20, 0x79, 0xa7, 0x4f, 0x38, 0xf3, 0x0f, 0xa0, 0x59,
    0xdc, 0x86, 0x7a, 0x74, 0xeb, 0x74, 0xbf, 0x7d, 0xf7, 0x6b, 0xf4, 0x02, 0xac, 0x84, 0x67, 0x09,
    0x0c, 0x82, 0x16, 0xde, 0xce, 0x7c, 0x78, 0x00, 0xe1, 0x99, 0x92, 0x36, 0xa4, 0x8f, 0x89, 0x1f,
    0x05, 0x4d, 0xa7, 0x57, 0xe7, 0x45, 0x2b, 0xbc, 0xc4, 0xd8, 0xbf, 0x0d, 0xba, 0xfc, 0x09, 0x5e,
    0x41, 0x90, 0x0f, 0x00, 0xfe, 0xa4, 0x0a, 0xb2, 0x6b, 0xe3, 0x36, 0x0c, 0x9f, 0xe2, 0x7b, 0x0e,
    0x96, 0xcd, 0x63, 0x03, 0xe4, 0xba, 0x9d, 0x30, 0x01, 0x13, 0x04, 0xdc, 0xfa, 0x7a, 0x30, 0x53,
    0x6e, 0xd7, 0x53, 0x48, 0xb1, 0x01, 0xd3, 0x08, 0x2e, 0x44, 0x73, 0xda, 0x25, 0x94, 0xef, 0xa1,
    0xe7, 0x0c, 0x56, 0x7d, 0x17, 0xf2, 0x94, 0xbf, 0xed, 0xca, 0xa6, 0x60, 0x06, 0x89, 0xd0, 0xac,
    0x58, 0x56, 0x08, 0xa2, 0x06, 0x89, 0xe6, 0xc2, 0xdf, 0x55, 0x3d, 0xb4, 0x34, 0x52, 0x9b, 0x59,
    0x82, 0x2e, 0x85, 0x8a, 0x3d, 0x51, 0x9d, 0x1a, 0x29, 0x2f, 0x54, 0x5d, 0x7e, 0x26, 0xe6, 0x24,
    0x9e, 0x23, 0x2e, 0x30, 0x8b, 0xe6, 0x78, 0x9d, 0x0e, 0x24, 0x44, 0xf0, 0x57, 0x05, 0x65, 0x7a,
    0x39, 0x37, 0x97, 0xf7, 0xc7, 0xb9, 0x9a, 0xcd, 0x68, 0xf1, 0xce, 0x50, 0x77, 0xfd, 0x25, 0x97
};

static esp_err_t get_sec2_salt(const char **salt, uint16_t *len)
{
#ifndef SMAK_PROD
    *salt = sec2_salt;
    *len  = sizeof(sec2_salt);
    return ESP_OK;
#else
    ESP_LOGE(TAG, "Salt not implemented");
    return ESP_FAIL;
#endif
}

static esp_err_t get_sec2_verifier(const char **verifier, uint16_t *len)
{
#ifndef SMAK_PROD
    *verifier = sec2_verifier;
    *len      = sizeof(sec2_verifier);
    return ESP_OK;
#else
    ESP_LOGE(TAG, "sec2 verifier not implemented")
    return ESP_FAIL;
#endif
}

static const int WIFI_CONNECTED_EVENT = BIT(0);
static EventGroupHandle_t smak_wifi_event_group;

#define PROV_QR_VERSION "v1"
#define PROV_TRANSPORT_SOFTAP "softap"
#define PROV_TRANSPORT_BLE "ble"
#define QRCODE_BASE_URL "https://espressif.github.io/esp-jumpstart/qrcode.html"

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == NETWORK_PROV_EVENT) {
        switch (event_id) {
        case NETWORK_PROV_START:
            ESP_LOGD(TAG, "Provisioning started");
            break;
        case NETWORK_PROV_WIFI_CRED_RECV: {
            wifi_sta_config_t *cfg = (wifi_sta_config_t *)event_data;
            ESP_LOGD(TAG, "Received Wi-Fi credentials for SSID: %s", cfg->ssid);
            break;
        }
        case NETWORK_PROV_WIFI_CRED_FAIL: {
            network_prov_wifi_sta_fail_reason_t *reason = (network_prov_wifi_sta_fail_reason_t *)event_data;
            ESP_LOGE(TAG,
                     "Provisioning failed!\n\tReason : %s"
                     "\n\tPlease reset to factory and retry provisioning",
                     (*reason == NETWORK_PROV_WIFI_STA_AUTH_ERROR)
                         ? "Wi-Fi station authentication failed"
                         : "Wi-Fi access-point not found");
            break;
        }
        case NETWORK_PROV_WIFI_CRED_SUCCESS:
            ESP_LOGD(TAG, "Provisioning successful");
            break;
        case NETWORK_PROV_END: {
            esp_err_t res = network_prov_mgr_deinit();
            if (res != ESP_OK) {
                ESP_LOGE(TAG, "Failed to de-initialize provisioning manager: %s",
                         esp_err_to_name(res));
            }
            break;
        }
        default:
            break;
        }
    } else if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
            esp_wifi_connect();
            break;
        default:
            break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR,
                 IP2STR(&event->ip_info.ip));
        /* Signal main application to continue execution */
        xEventGroupSetBits(smak_wifi_event_group, WIFI_CONNECTED_EVENT);

    } else if (event_base == PROTOCOMM_SECURITY_SESSION_EVENT) {
        switch (event_id) {
        case PROTOCOMM_SECURITY_SESSION_SETUP_OK:
            ESP_LOGI(TAG, "Secured session established!");
            break;
        case PROTOCOMM_SECURITY_SESSION_INVALID_SECURITY_PARAMS:
            ESP_LOGE(TAG, "Received invalid security parameters for establishing "
                          "secure session!");
            break;
        case PROTOCOMM_SECURITY_SESSION_CREDENTIALS_MISMATCH:
            ESP_LOGE(TAG, "Received incorrect username and/or PoP for establishing "
                          "secure session!");
            break;
        default:
            break;
        }
    }
}

static void smak_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void smak_device_name_get(char *service_name, size_t max)
{
    uint8_t eth_mac[6];
    const char *ssid_prefix = "PROV_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "%s%02X%02X%02X", ssid_prefix, eth_mac[3],
             eth_mac[4], eth_mac[5]);
}

static void smak_wifi_prov_qr_print(const char *name, const char *username,
                                    const char *pop, const char *transport)
{
    if (!name || !transport) {
        ESP_LOGW(TAG, "Cannot generate QR code payload. Data missing.");
        return;
    }
    char payload[256] = { 0 };
    if (pop) {
        snprintf(payload, sizeof(payload),
                 "{\"ver\":\"%s\",\"name\":\"%s\""
                 ",\"username\":\"%s\",\"pop\":\"%s\",\"transport\":\"%s\"}",
                 PROV_QR_VERSION, name, username, pop, transport);
    } else {
        snprintf(payload, sizeof(payload),
                 "{\"ver\":\"%s\",\"name\":\"%s\""
                 ",\"transport\":\"%s\",\"network\":\"wifi\"}",
                 PROV_QR_VERSION, name, transport);
    }
    ESP_LOGI(TAG, "SOFTAP WIFI PROVISIONING QR CODE");
    esp_qrcode_config_t qr_cfg = ESP_QRCODE_CONFIG_DEFAULT();
    esp_qrcode_generate(&qr_cfg, payload);
}

void softap_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_ERROR_CHECK(nvs_flash_erase());

        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    smak_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_handler_register(
        NETWORK_PROV_EVENT,
        ESP_EVENT_ANY_ID,
        event_handler,
        NULL));

    ESP_ERROR_CHECK(esp_event_handler_register(
        PROTOCOMM_SECURITY_SESSION_EVENT,
        ESP_EVENT_ANY_ID, event_handler,
        NULL));

    ESP_ERROR_CHECK(esp_event_handler_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP,
        event_handler, NULL));

    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));

    network_prov_mgr_config_t config = {
        .scheme               = network_prov_scheme_softap,
        .scheme_event_handler = NETWORK_PROV_EVENT_HANDLER_NONE
    };

    ESP_ERROR_CHECK(network_prov_mgr_init(config));

    bool is_provisioned = false;

    ESP_ERROR_CHECK(network_prov_mgr_is_wifi_provisioned(&is_provisioned));
    if (!is_provisioned) {
        ESP_LOGI(TAG, "Starting provisioning");

        char service_name[12];
        smak_device_name_get(service_name, sizeof(service_name));

        const char *username                        = SEC2_USERNAME;
        const char *pop                             = SEC2_PWD;
        network_prov_security2_params_t sec2_params = {};

        ESP_ERROR_CHECK(get_sec2_salt(&sec2_params.salt, &sec2_params.salt_len));
        ESP_ERROR_CHECK(get_sec2_verifier(&sec2_params.verifier,
                                          &sec2_params.verifier_len));

        // network_prov_security2_params_t *sec_params = &sec2_params;

        const char *service_key = NULL;
        ESP_ERROR_CHECK(network_prov_mgr_start_provisioning(
            NETWORK_PROV_SECURITY_2,
            (const void *)&sec2_params,
            service_name,
            service_key));

        smak_wifi_prov_qr_print(service_name, username, pop, PROV_TRANSPORT_SOFTAP);
    } else {
        ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");

        ESP_ERROR_CHECK(network_prov_mgr_deinit());

        ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                   &event_handler, NULL));
        smak_wifi_init();
    }

    xEventGroupWaitBits(smak_wifi_event_group, WIFI_CONNECTED_EVENT, true, true,
                        portMAX_DELAY);

    return;
}
