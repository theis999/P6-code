#include "model.h"
#include "pb.h"
#include <pb_decode.h>
#include <pb_encode.h>
#include <src/smak.pb.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/data/json.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(model, LOG_LEVEL_DBG);

#define CREATE_STATE_STRING_ARRAY(name) [name] = #name

const char *state_strings[] = { FOR_EACH(
    CREATE_STATE_STRING_ARRAY, (, ), white, white_move, white_capture,
    white_enemy_capture, white_castling, white_castling_kingside_kingdown,
    white_castling_queenside_kingdown, white_castling_kingside_KINGUP_ROOKUP,
    white_castling_queenside_KINGUP_ROOKUP,
    white_castling_kingside_kingdown_ROOKUP,
    white_castling_queenside_kingdown_ROOKUP,
    white_castling_kingside_KINGUP_rookdown,
    white_castling_queenside_KINGUP_rookdown, black, black_move, black_capture,
    black_enemy_capture, black_castling, black_castling_kingside_kingdown,
    black_castling_queenside_kingdown, black_castling_kingside_KINGUP_ROOKUP,
    black_castling_queenside_KINGUP_ROOKUP,
    black_castling_kingside_kingdown_ROOKUP,
    black_castling_queenside_kingdown_ROOKUP,
    black_castling_kingside_KINGUP_rookdown,
    black_castling_queenside_KINGUP_rookdown, error) };

#undef CREATE_STATE_STRING_ARRAY

/* clang-format off */
[[gnu::unused]] const char board[64] = {
    'r','n','b','q','k','b','n','r',
    'p','p','p','p','p','p','p','p',
    ' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',
    'P','P','P','P','P','P','P','P',
    'R','N','B','Q','K','B','N','R',
};

const char starting_board_initializer[64] = {
    'r','n','b','q','k','b','n','r',
    'p','p','p','p','p','p','p','p',
    ' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',
    ' ',' ',' ',' ',' ',' ',' ',' ',
    'P','P','P','P','P','P','P','P',
    'R','N','B','Q','K','B','N','R',
};

const char empty_board_initializer[64] = {
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
};

/* clang-format on */

struct smak_json_obj_internal {
    uint64_t id;
    uint32_t ply;
    uint16_t from;
    uint16_t to;
    uint8_t piece[2];
    uint8_t captured[2];
    const uint8_t *move_type;
};

static const char *move_type_strings[] = { [EN_PASSANT] = "enpassant",
                                           [CASTLING]   = "castling",
                                           [PROMOTION]  = "promotion",
                                           [NORMAL]     = "normal" };

/* clang-format off */

static const struct json_obj_descr desc_array[] = {
    JSON_OBJ_DESCR_PRIM(struct smak_json_obj_internal, id,        JSON_TOK_INT64),
    JSON_OBJ_DESCR_PRIM(struct smak_json_obj_internal, ply,       JSON_TOK_INT),
    JSON_OBJ_DESCR_PRIM(struct smak_json_obj_internal, from,      JSON_TOK_INT),
    JSON_OBJ_DESCR_PRIM(struct smak_json_obj_internal, to,        JSON_TOK_INT),
    JSON_OBJ_DESCR_PRIM(struct smak_json_obj_internal, piece,     JSON_TOK_STRING_BUF),
    JSON_OBJ_DESCR_PRIM(struct smak_json_obj_internal, move_type, JSON_TOK_STRING)
};

/* clang-format on */

void encode_smak_obj(struct smak_json_obj *obj, char *buf, size_t buf_size)
{

    struct smak_json_obj_internal obj_internal = {
        .id        = obj->id,
        .ply       = obj->ply,
        .from      = obj->from,
        .to        = obj->to,
        .piece     = { obj->piece,    '\0' },
        .captured  = { obj->captured, '\0' },
        .move_type = move_type_strings[obj->movetype],
    };

    json_obj_encode_buf(desc_array, ARRAY_SIZE(desc_array), &obj_internal, buf,
                        buf_size);
}

uint8_t pin(const char *restrict str)
{
    char file_char = *str;
    char rank_char = *(str + 1);

    uint8_t file = (file_char - 'a');
    uint8_t rank = (rank_char - '0');
    uint8_t r    = (8 - rank) << 3;

    return (r | file);
}

int smak_move_to_pb(struct smak_json_obj *obj, uint8_t *buf)
{
    Smak_ChessMove mv = {
        .id        = obj->id,
        .ply       = obj->ply,
        .from      = obj->from,
        .to        = obj->to,
        .piece     = { .size = 1, { obj->piece }                                 },
        .captured  = { .size = 1, { obj->captured == ' ' ? 'Z' : obj->captured } },
        .move_type = (Smak_MoveType)obj->movetype,
    };
    pb_ostream_t stream = pb_ostream_from_buffer(buf, Smak_ChessMove_size);
    bool success        = pb_encode(&stream, Smak_ChessMove_fields, &mv);
    size_t encoded_size = stream.bytes_written;
    LOG_DBG("Encoded size is %u bytes", encoded_size);
    if (success) {
        return encoded_size;
    } else {
        return 0;
    }
}
