#include "cJSON.h"
#include <ctype.h>
#include <esp_log.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include <smak.pb.h>
#include <smak_defines.h>
#include <smak_util.h>
#include <smak_util_private.h>

char *smak_json_game_obj_to_str(const struct smak_json_game_obj *in)
{
#define CHECK_NOT_NULL_OR_GOTO_OUT(x)            \
    if (!(x)) {                                  \
        ESP_LOGE(__func__, #x " returned NULL"); \
        goto out;                                \
    }

    char *str_out = { 0 };

    cJSON *smak_game_obj = cJSON_CreateObject();
    CHECK_NOT_NULL_OR_GOTO_OUT(smak_game_obj);
    CHECK_NOT_NULL_OR_GOTO_OUT(in);

    cJSON *board = cJSON_CreateObject();
    CHECK_NOT_NULL_OR_GOTO_OUT(board);

    CHECK_NOT_NULL_OR_GOTO_OUT(cJSON_AddNumberToObject(board, "id", in->board.id));
    CHECK_NOT_NULL_OR_GOTO_OUT(cJSON_AddItemToObject(smak_game_obj, "board", board));
    CHECK_NOT_NULL_OR_GOTO_OUT(cJSON_AddStringToObject(smak_game_obj, "gamestate", smak_gamestate_strings[in->gamestate]));

    str_out = cJSON_Print(smak_game_obj);
    CHECK_NOT_NULL_OR_GOTO_OUT(str_out);
out:
    cJSON_Delete(smak_game_obj);
    return str_out;
#undef CHECK_NOT_NULL_OR_GOTO_OUT
}

const char *move_type_strings[] = {
    [EN_PASSANT] = "enpassant",
    [CASTLING]   = "castling",
    [PROMOTION]  = "promotion",
    [NORMAL]     = "normal"
};

const char *smak_gamestate_strings[] = {
    [WHITE_WIN]        = "WHITE_WIN",
    [BLACK_WIN]        = "BLACK_WIN",
    [DRAW_AGREED]      = "DRAW_AGREED",
    [DRAW_REPETITION]  = "DRAW_REPETITION",
    [DRAW_FIFTY_MOVES] = "DRAW_FIFTY_MOVES",
    [WHITE_TO_MOVE]    = "WHITE_TO_MOVE",
    [BLACK_TO_MOVE]    = "BLACK_TO_MOVE",
    [NOT_STARTED]      = "NOT_STARTED",
    [ERROR]            = "ERROR"
};

int smak_json_move_obj_internal_get_from_pb(smak_chess_move_t *in, size_t mv_sz, struct smak_json_move_obj *out)
{

    if (!in) {
        SMAK_LOGE("NULL parameter");
        return -1;
    }
    out->id          = in->id;
    out->ply         = in->ply;
    out->from        = in->from;
    out->to          = in->to;
    out->piece[0]    = toupper(in->piece.bytes[0]);
    out->piece[1]    = '\0';
    out->captured[0] = toupper(in->captured.bytes[0]);
    out->captured[1] = '\0';
    out->move_type   = (const uint8_t *)move_type_strings[in->move_type];

    return 0;
}

char *smak_json_move_obj_to_str(struct smak_json_move_obj *obj)
{
#define CHECK_NOT_NULL_OR_GOTO_OUT(x)           \
    if (!(x)) {                                 \
        ESP_LOGE(__func__, #x "returned NULL"); \
        goto out;                               \
    }

    char *str_out        = { 0 };
    cJSON *smak_move_obj = { 0 };

    CHECK_NOT_NULL_OR_GOTO_OUT(obj);

    smak_move_obj = cJSON_CreateObject();
    CHECK_NOT_NULL_OR_GOTO_OUT(smak_move_obj);

    CHECK_NOT_NULL_OR_GOTO_OUT(
        cJSON_AddNumberToObject(smak_move_obj, SMAK_MOVE_JSON_FIELD_ID, obj->id));

    CHECK_NOT_NULL_OR_GOTO_OUT(
        cJSON_AddNumberToObject(smak_move_obj, SMAK_MOVE_JSON_FIELD_PLY_NUMBER, obj->ply));

    CHECK_NOT_NULL_OR_GOTO_OUT(
        cJSON_AddNumberToObject(smak_move_obj, SMAK_MOVE_JSON_FIELD_FROM_SQUARE, obj->from));

    CHECK_NOT_NULL_OR_GOTO_OUT(
        cJSON_AddNumberToObject(smak_move_obj, SMAK_MOVE_JSON_FIELD_TO_SQUARE, obj->to));

    CHECK_NOT_NULL_OR_GOTO_OUT(
        cJSON_AddStringToObject(smak_move_obj, SMAK_MOVE_JSON_FIELD_PIECE_MOVED, (const char *)obj->piece));

    CHECK_NOT_NULL_OR_GOTO_OUT(
        cJSON_AddStringToObject(smak_move_obj, SMAK_MOVE_JSON_FIELD_PIECE_CAPTURED, (const char *)obj->captured));

    CHECK_NOT_NULL_OR_GOTO_OUT(
        cJSON_AddStringToObject(smak_move_obj, SMAK_MOVE_JSON_FIELD_MOVE_TYPE, (const char *)obj->move_type));

    CHECK_NOT_NULL_OR_GOTO_OUT(
        (str_out = cJSON_Print(smak_move_obj)));
out:
    cJSON_Delete(smak_move_obj);
    return str_out;
#undef CHECK_NOT_NULL_OR_GOTO_OUT
}
