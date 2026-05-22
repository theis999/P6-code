#ifndef SMAK_INCLUDED_SMAK_DEFINES_H_
#define SMAK_INCLUDED_SMAK_DEFINES_H_

#include <esp_bit_defs.h>
#include <esp_log.h>
#include <sdkconfig.h>

#define SMAK_MOVE_JSON_FIELD_ID "id"
#define SMAK_MOVE_JSON_FIELD_PLY_NUMBER "ply_number"
#define SMAK_MOVE_JSON_FIELD_FROM_SQUARE "from_square"
#define SMAK_MOVE_JSON_FIELD_TO_SQUARE "to_square"
#define SMAK_MOVE_JSON_FIELD_PIECE_MOVED "piece_moved"
#define SMAK_MOVE_JSON_FIELD_PIECE_CAPTURED "piece_captured"
#define SMAK_MOVE_JSON_FIELD_MOVE_TYPE "move_type"

#define SMAK_GAME_ID_NVS_KEY "game_id"

#define WIFI_CONNECTED_BIT BIT(0)
#define WIFI_CONN_FAIL_BIT BIT(1)

#define SMAK_ENDPOINT_URL_CREATE(subdomain, domain, url) "https://" subdomain "." domain url

// #define SMAK_ENDPOINT_AUTH_TOKEN_GET "https://smakauth.head9x.dk/application/o/token/"
// #define SMAK_ENDPOINT_DB_GAME_POST "https://smakdb.head9x.dk/games"
// #define SMAK_ENDPOINT_DB_MOVE_POST "https://smakdb.head9x.dk/moves"
#define SMAK_LOGD(fmt, ...) ESP_LOGD(__FILE_NAME__, "%s:%d: " fmt, __func__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#define SMAK_LOGI(fmt, ...) ESP_LOGI(__FILE_NAME__, "%s:%d: " fmt, __func__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#define SMAK_LOGW(fmt, ...) ESP_LOGW(__FILE_NAME__, "%s:%d: " fmt, __func__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#define SMAK_LOGE(fmt, ...) ESP_LOGE(__FILE_NAME__, "%s:%d: " fmt, __func__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#define ARG_UNUSED(x) (void)x

#define var_unused __attribute__((unused))

#define TASK_DELETE_SELF() vTaskDelete(NULL)

#endif // SMAK_INCLUDED_SMAK_DEFINES_H_
