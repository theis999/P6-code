#include "esp_err.h"
#include "esp_log.h"

#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "pb_decode.h"
#include "pb_encode.h"
#include "portmacro.h"
#include "sdkconfig.h"
#include "smak_defines.h"
#include <complex.h>
#include <esp_crt_bundle.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <time.h>

#include <smak_util.h>

#include <cJSON.h>
#include <esp_http_client.h>
#include <esp_timer.h>
#include <pb.h>
#include <smak.pb.h>
#include <smak_http_interface.h>
#include <smak_storage.h>
#include <smak_util_private.h>

extern uint64_t board_id;

void test_smak_json_print(void)
{

    smak_chess_move_t move = {
        .move_type = SMAK_MOVE_TYPE_SMAKMOVE_NORMAL,
        .from      = 52,
        .to        = 36,
        .id        = 3,
        .ply       = 1,
        .piece     = { .size = 1, .bytes = { 'P' } },
        .captured  = { .size = 1, .bytes = { 'Z' } }
    };

    char buffer[SMAK_CHESS_MOVE_SIZE] = { 0 };

    SMAK_LOGI("Encoding PB");
    pb_ostream_t ostream = pb_ostream_from_buffer((pb_byte_t *)buffer, sizeof(buffer));
    pb_encode(&ostream, SMAK_CHESS_MOVE_FIELDS, &move);
    size_t len = ostream.bytes_written;
    SMAK_LOGI("Encoded length is %u bytes", len);
    SMAK_LOGI("Decoding");

    smak_chess_move_t decoded = SMAK_CHESS_MOVE_INIT_ZERO;
    pb_istream_t istream      = pb_istream_from_buffer((pb_byte_t *)buffer, len);
    pb_decode(&istream, SMAK_CHESS_MOVE_FIELDS, &decoded);

    SMAK_LOGI("id: %llu; ply: %u; from: %u; to: %u; piece_moved: %c; piece_captured: %c; move_type: %s;",
              decoded.id, decoded.ply, decoded.from, decoded.to, decoded.piece.bytes[0], decoded.captured.bytes[0], move_type_strings[decoded.move_type]);

    SMAK_LOGI("%s",
              smak_json_move_obj_to_str(&(struct smak_json_move_obj) {
                  .captured  = "p",
                  .piece     = "p",
                  .from      = 36,
                  .to        = 56,
                  .ply       = 3,
                  .id        = 0,
                  .move_type = (const uint8_t *)move_type_strings[NORMAL] }));
}

static const char *endpoints[] = {
    [SMAK_HTTP_MOVE_POST]      = SMAK_ENDPOINT_URL_CREATE(CONFIG_SMAK_HTTP_DB_SUBDOMAIN, CONFIG_SMAK_HTTP_HOSTNAME_DOMAIN, CONFIG_SMAK_HTTP_DB_MOVE_POST_URL),
    [SMAK_HTTP_GAME_POST]      = SMAK_ENDPOINT_URL_CREATE(CONFIG_SMAK_HTTP_DB_SUBDOMAIN, CONFIG_SMAK_HTTP_HOSTNAME_DOMAIN, CONFIG_SMAK_HTTP_DB_GAME_POST_URL),
    [SMAK_HTTP_GAME_PATCH]     = SMAK_ENDPOINT_URL_CREATE(CONFIG_SMAK_HTTP_DB_SUBDOMAIN, CONFIG_SMAK_HTTP_HOSTNAME_DOMAIN, CONFIG_SMAK_HTTP_DB_GAME_PATCH_URL),
    [SMAK_HTTP_AUTH_TOKEN_GET] = SMAK_ENDPOINT_URL_CREATE(CONFIG_SMAK_HTTP_AUTH_SUBDOMAIN, CONFIG_SMAK_HTTP_HOSTNAME_DOMAIN, CONFIG_SMAK_HTTP_AUTH_TOKEN_GET_URL),
    [SMAK_HTTP_USERS_GET]      = NULL
};

static EventGroupHandle_t token_event_group = { 0 };

/// @todo Gem den her i encrypted NVS
static const char secret[] = CONFIG_SMAK_CLIENT_CREDENTIALS_SECRET_STRING;

static bool token_valid = false;

struct {
    char buf[4096];
    size_t len;
} token_info = { 0 };

[[maybe_unused]] static const size_t bearer_offset = sizeof("Bearer ");

cJSON *token_global = { 0 };

static char token_global_str[4096] = { 'B', 'e', 'a', 'r', 'e', 'r', ' ', '\0' };

#define HTTP_BUFFER_SIZE 4096

int token_alloc(char *tok, size_t tok_size)
{

    return 0;
}

static esp_err_t http_auth_event_handler(esp_http_client_event_t *event)
{
    static char *resp_buf   = { 0 };
    static size_t resp_size = { 0 };

    switch (event->event_id) {

    case HTTP_EVENT_ERROR:
        ESP_LOGE(__func__, "HTTP error");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(__func__, "Connected");
        break;
    case HTTP_EVENT_HEADERS_SENT:
        ESP_LOGI(__func__, "Headers sent");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(__func__, "Received header: %s: %s", event->header_key, event->header_value);
        break;
    case HTTP_EVENT_ON_HEADERS_COMPLETE:
        ESP_LOGI(__func__, "Headers complete");
        break;
    case HTTP_EVENT_ON_STATUS_CODE:
        ESP_LOGI(__func__, "Status code: %d", esp_http_client_get_status_code(event->client));
        break;
    case HTTP_EVENT_ON_DATA: {
        ESP_LOGI(__func__, "Data event");
        ESP_LOGI(__func__, "Data length: %i", event->data_len);
        // if (resp_size == 0 && event->user_data) {
        //     memset(*(char **)event->user_data, 0, HTTP_BUFFER_SIZE);
        // }

        // ESP_LOG_BUFFER_HEXDUMP(__func__, event->data, event->data_len, ESP_LOG_INFO);

        if (esp_http_client_is_chunked_response(event->client)) {
            if (resp_size == 0) {
                token_info.len = 0;
            }
            int chunk_size = { 0 };

            esp_http_client_get_chunk_length(event->client, &chunk_size);
            ESP_LOGI(__func__, "Chunk-Size is %d", chunk_size);

            memcpy(&token_info.buf[resp_size], event->data, event->data_len);
            resp_size += event->data_len;
            token_info.len = resp_size;
        }

        if (!esp_http_client_is_chunked_response(event->client)) {
            size_t len = 0;
            if (event->user_data) {
                if (event->data_len > HTTP_BUFFER_SIZE) {
                    *(char **)event->user_data = realloc(*(char **)event->user_data, HTTP_BUFFER_SIZE * 2);
                }
                len = event->data_len < (HTTP_BUFFER_SIZE - resp_size) ? event->data_len : (HTTP_BUFFER_SIZE - resp_size);
                if (len) {
                    memcpy(*(char **)event->user_data + resp_size, event->data, len);
                }
            } else {
                size_t content_len = esp_http_client_get_content_length(event->client);

                resp_buf  = calloc(content_len + 1, sizeof(char));
                resp_size = 0;
                if (resp_buf == NULL) {
                    ESP_LOGE(__func__, "NO MEMORY LEFT");
                    return ESP_FAIL;
                }
                resp_size = event->data_len < (content_len - resp_size) ? event->data_len : (content_len - resp_size);
                if (resp_size) {
                    memcpy(resp_buf + resp_size, event->data, len);
                }
            }
            resp_size += len;
        }
        break;
    }
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(__func__, "Finished data transfer");
        if (resp_buf != NULL) {
            free(resp_buf);
            resp_buf = NULL;
        }

        if (!esp_http_client_is_chunked_response(event->client)) {
            memcpy(token_info.buf, *(char **)event->user_data, resp_size);
            free(*(char **)event->user_data);
            *(char **)event->user_data = NULL;
        }

        resp_size   = 0;
        token_valid = true;
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(__func__, "Disconnected");
        if (resp_buf != NULL) {
            free(resp_buf);
            resp_buf = NULL;
        }
        resp_size = 0;

        if (*(char **)event->user_data != NULL) {
            free(*(char **)event->user_data);
            *(char **)event->user_data = NULL;

            resp_size   = 0;
            token_valid = false;
        }
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGI(__func__, "Redirecting");
        /// @todo Redirecter vi?
        break;
    }

    return ESP_OK;
};

char *smak_http_auth_token_get(const char *host)
{

    char *token_resp_buf = calloc(HTTP_BUFFER_SIZE + 1, sizeof(char));

    if (!token_resp_buf) {
        return NULL;
    }
    esp_http_client_config_t c_cfg = {
        .host              = host,
        .url               = endpoints[SMAK_HTTP_AUTH_TOKEN_GET],
        .crt_bundle_attach = esp_crt_bundle_attach,
        .method            = HTTP_METHOD_POST,
        .event_handler     = http_auth_event_handler,
        .user_data         = &token_resp_buf,
        .user_agent        = "SMAK-esp",
    };

    esp_http_client_handle_t client = esp_http_client_init(&c_cfg);
    esp_http_client_set_post_field(client, secret, sizeof(secret));
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_perform(client);

    token_info.buf[token_info.len] = '\0';
    SMAK_LOGI("Token has length: %d", token_info.len);
    SMAK_LOGI("%.*s", token_info.len, token_info.buf);

    if (token_global) {
        /// @todo mutex
        cJSON_free(token_global);
    }

    token_global   = cJSON_Parse(token_info.buf);
    cJSON *tok_val = cJSON_GetObjectItem(token_global, "access_token");
    char *str_val  = cJSON_GetStringValue(tok_val);

    SMAK_LOGI("%s", str_val);

    if (!str_val) {
        SMAK_LOGE("No token");
        return NULL;
    }
    SMAK_LOGD("strcat proceeding");
    strcat(token_global_str, str_val);
    SMAK_LOGD("strcat completed");

    esp_http_client_cleanup(client);

    return token_global_str;
}

static esp_err_t smak_http_event_handler_post(esp_http_client_event_t *event)
{

    static size_t idx = { 0 };

    char *buf = event->user_data;

    switch (event->event_id) {

    case HTTP_EVENT_ON_DATA: {

        SMAK_LOGI("Received %d bytes", event->data_len);

        if ((HTTP_BUFFER_SIZE - idx) < event->data_len) {
            SMAK_LOGE("No space in receive buffer");
            return ESP_FAIL;
        }
        // ESP_LOG_BUFFER_HEXDUMP(__func__, &buf[idx], event->data_len, ESP_LOG_INFO);
        memcpy(&buf[idx], event->data, event->data_len);
        idx += event->data_len;
        break;
    }
    case HTTP_EVENT_ERROR:
        SMAK_LOGE("Error");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        SMAK_LOGI("Connected");
        break;
    case HTTP_EVENT_HEADERS_SENT:
        SMAK_LOGI("Headers sent");
        break;
    case HTTP_EVENT_ON_HEADER:
        SMAK_LOGI("Received header: %s: %s", event->header_key, event->header_value);
        break;
    case HTTP_EVENT_ON_HEADERS_COMPLETE:
        SMAK_LOGI("All headers received");
        break;
    case HTTP_EVENT_ON_STATUS_CODE:
        SMAK_LOGI("Status code: %d", esp_http_client_get_status_code(event->client));
        break;
    case HTTP_EVENT_ON_FINISH:
        SMAK_LOGI("%.*s", idx, buf);
        buf[idx] = '\0';
        idx      = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        SMAK_LOGI("Disconnected");
        break;
    case HTTP_EVENT_REDIRECT:
        SMAK_LOGI("Redirected");
        break;
    }

    return ESP_OK;
}

void smak_http_post_game(const struct smak_json_game_obj *obj)
{
    SMAK_LOGI("Posting game");
    static char resp_buf[HTTP_BUFFER_SIZE + 1] = { 0 };

    esp_http_client_config_t c_cfg = {
        .url               = endpoints[SMAK_HTTP_GAME_POST],
        .crt_bundle_attach = esp_crt_bundle_attach,
        .method            = HTTP_METHOD_POST,
        .event_handler     = smak_http_event_handler_post,
        .user_data         = resp_buf,
        .buffer_size_tx    = HTTP_BUFFER_SIZE,
        .user_agent        = "SMAK-esp",
        .timeout_ms        = 20000
    };

    char *game_str = smak_json_game_obj_to_str(obj);
    if (!game_str) {
        SMAK_LOGE("smak_json_game_obj_to_str() returned NULL");
        return;
    }

    SMAK_LOGI("Content: \n%s", game_str);
    esp_http_client_handle_t client = esp_http_client_init(&c_cfg);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", token_global_str);
    esp_http_client_set_post_field(client, game_str, strlen(game_str));

    esp_http_client_perform(client);

    cJSON *parsed = cJSON_Parse(resp_buf);
    if (!parsed) {
        SMAK_LOGE("Could not parse response JSON");
        goto out;
    }

    cJSON *id_obj = cJSON_GetObjectItem(parsed, "id");
    if (!id_obj) {
        SMAK_LOGE("Could not get \"id\" field from response JSON");
        goto free_json_resp;
    }

    uint64_t id = (uint64_t)cJSON_GetNumberValue(id_obj);

    SMAK_LOGI("Received id: %llu", id);
    SMAK_LOGI("Current NVS id: %llu", smak_gameid_current_get());

    if (id == smak_gameid_current_get()) {
        goto free_json_id;
    }

    int res = smak_gameid_store(id);
    if (res < 0) {
        SMAK_LOGE("Failed to save game ID");
    }

    SMAK_LOGI("Current game ID is %llu", smak_gameid_current_get());

[[maybe_unused]] free_json_id:
    cJSON_free(id_obj);
free_json_resp:
    cJSON_free(parsed);
out:
    esp_http_client_cleanup(client);

    free(game_str);
}

void smak_http_patch_game(uint64_t id, const struct smak_json_game_obj *obj)
{
    SMAK_LOGI("Patching game");
    static char resp_buf[HTTP_BUFFER_SIZE + 1] = { 0 };

    esp_http_client_config_t c_cfg = {
        .url               = endpoints[SMAK_HTTP_GAME_PATCH],
        .crt_bundle_attach = esp_crt_bundle_attach,
        .method            = HTTP_METHOD_PATCH,
        .event_handler     = smak_http_event_handler_post,
        .user_data         = resp_buf,
        .buffer_size_tx    = HTTP_BUFFER_SIZE,
        .user_agent        = "SMAK-esp",
        .timeout_ms        = 20000
    };

    char *game_str = smak_json_game_obj_to_str(obj);
    if (!game_str) {
        SMAK_LOGE("smak_json_game_obj_to_str() returned NULL");
        return;
    }

    SMAK_LOGI("Content: \n%s", game_str);
    esp_http_client_handle_t client = esp_http_client_init(&c_cfg);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", token_global_str);
    esp_http_client_set_post_field(client, game_str, strlen(game_str));

    esp_http_client_perform(client);
    esp_http_client_cleanup(client);

    free(game_str);
}

void smak_http_post_move(uint64_t id, smak_chess_move_t *move, size_t pb_size)
{

    SMAK_LOGI("current id in nvs: %llu", smak_gameid_current_get());

    static char resp_buf[HTTP_BUFFER_SIZE + 1] = { 0 };

    struct smak_json_move_obj json_out;
    int res = smak_json_move_obj_internal_get_from_pb(move, pb_size, &json_out);
    if (res < 0) {
        SMAK_LOGE("smak_json_move_obj_internal_get_from_pb() failed");
        return;
    }

    esp_http_client_config_t c_cfg = {
        .url               = endpoints[SMAK_HTTP_MOVE_POST],
        .crt_bundle_attach = esp_crt_bundle_attach,
        .user_agent        = "SMAK-esp",
        .buffer_size_tx    = HTTP_BUFFER_SIZE,
        .event_handler     = smak_http_event_handler_post,
        .user_data         = resp_buf,
        .method            = HTTP_METHOD_POST,
        .timeout_ms        = 20000
    };

    char *move_str = smak_json_move_obj_to_str(&json_out);
    if (!move_str) {
        SMAK_LOGE("smak_json_move_obj_to_str() returned NULL");
        return;
    }

    esp_http_client_handle_t client = esp_http_client_init(&c_cfg);

    if (!client) {
        SMAK_LOGE("Client init failed");
        return;
    }

    SMAK_LOGI("Content: \n %s", move_str);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Authorization", token_global_str);
    esp_http_client_set_post_field(client, move_str, strlen(move_str));

    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}

void call_post(void)
{
    struct smak_json_game_obj a = { .gamestate = WHITE_TO_MOVE, .board.id = 1 };

    if (token_info.len == 0) {
        SMAK_LOGE("No token");
        return;
    }
    smak_http_post_game(&a);
    /* clang-format off */
    smak_chess_move_t mv = {
        .id        = 3,
        .move_type = SMAK_MOVE_TYPE_SMAKMOVE_NORMAL,
        .piece     = (smak_chess_move_piece_t) {
            .bytes[0] = 'P',
            .size     = 1,
        },
        .captured = (smak_chess_move_captured_t) {
            .bytes[0] = 'Z',
            .size     = 1,
        },
        .from = 52,
        .to = 36,
        .ply = 9,                           
    };

    /* clang-format on */

    smak_http_post_move(3, &mv, 1);
}
static void smak_token_get_task(void *arg)
{

#define MICROSECONDS_PER_MINUTE 30'000'000

    TickType_t last_woken_at            = { 0 };
    const TickType_t five_mins_in_ticks = pdMS_TO_TICKS(5 * MICROSECONDS_PER_MINUTE);

    while (1) {
        vTaskDelayUntil(&last_woken_at, five_mins_in_ticks);
        char *result = smak_http_auth_token_get(endpoints[SMAK_HTTP_AUTH_TOKEN_GET]);
        if (!result) {
            SMAK_LOGW("Failed to get auth token");
        }
    }
}

TaskFunction_t get_smak_token_task(void)
{
    return smak_token_get_task;
};

#define SMAK_CONNECTIVITY_TASK_STACK_SIZE_REQUIRED 32784

QueueHandle_t smak_http_req_queue = { 0 };

QueueHandle_t init_smak_http_req_queue()
{

    smak_http_req_queue = xQueueCreate(8, sizeof(struct smak_http_command));

    return smak_http_req_queue;
}

void smak_connectivity_task(void *arg)
{
    SMAK_LOGD("Task starting");

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    if (!smak_http_req_queue) {
        SMAK_LOGE("request queue not existing");
        vTaskDelete(NULL);
    }

    smak_http_command_t cmd = { 0 };

    while (1) {
        xQueueReceive(smak_http_req_queue, &cmd, portMAX_DELAY);

        SMAK_LOGI("Reading from queue");

        switch (cmd.cmd_type) {
        case SMAK_HTTP_MOVE_POST: {
            smak_http_post_move(cmd.game_id, &cmd.data.mv, 0);
            break;
        }
        case SMAK_HTTP_GAME_POST: {
            smak_http_post_game(&cmd.data.game_info.game);
            break;
        }
        case SMAK_HTTP_GAME_PATCH: {
            smak_http_patch_game(cmd.data.game_info.id, &cmd.data.game_info.game);
            break;
        }
        case SMAK_HTTP_AUTH_TOKEN_GET: {
            SMAK_LOGI("Getting auth token");
            if (!(smak_http_auth_token_get(endpoints[SMAK_HTTP_AUTH_TOKEN_GET]))) {
                SMAK_LOGW("smak_http_auth_token_get() failed");
            }
            break;
        }
        case SMAK_HTTP_USERS_GET:
            SMAK_LOGW("SMAK_HTTP_USERS_GET HTTP command is not implemented");
            break;
        default:
            SMAK_LOGW("Unknown SMAK HTTP command type (%04X)", cmd.cmd_type);
            break;
        }
    }
}

void smak_http_cmd_send(smak_http_command_t *cmd)
{
    if (smak_http_req_queue) {
        xQueueSend(smak_http_req_queue, cmd, 0);
    } else {
        SMAK_LOGE("Queue doesn't exist");
    }
}

int smak_conn_task_init(void)
{
    init_smak_http_req_queue();
    BaseType_t task_created = xTaskCreatePinnedToCore(smak_connectivity_task, "smak_connectivity_task", SMAK_CONNECTIVITY_TASK_STACK_SIZE_REQUIRED, NULL, 9, NULL, 0);

    if (!task_created) {
        SMAK_LOGE("Connectivity task could not be created");
#if 0
        SMAK_LOGE("Restarting");
        esp_restart();
#else
        return -1;
#endif
    }

    SMAK_LOGI("Connectivity task created");

    smak_http_command_t cmd = {
        .cmd_type = SMAK_HTTP_AUTH_TOKEN_GET,
    };

    SMAK_LOGI("Sending auth request");
    xQueueSend(smak_http_req_queue, &cmd, portMAX_DELAY);

    return 0;
}
