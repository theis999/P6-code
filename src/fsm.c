#include "fsm.h"
#include <pb.h>
#include <pb_decode.h>
#include <pb_encode.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>
#include <zephyr/data/json.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/thread_stack.h>
#include <zephyr/shell/shell_fprintf.h>
#include <zephyr/shell/shell_types.h>
#include <zephyr/smf.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smak_fsm, LOG_LEVEL_DBG);

#include <src/smak.pb.h>
#include <zephyr/toolchain.h>

#ifdef CONFIG_ZTEST
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>
#endif

#include "model.h"
#include <zephyr/cleanup.h>
#include <zephyr/cleanup/kernel.h>
#include <zephyr/sys/slist.h>

struct smak_json_obj shell_obj;

struct chess_state {
    struct smf_ctx ctx;
    bool is_up;
    bool is_down;
    uint16_t pin_number;
    bool is_empty_square;
    bool is_black_piece;
    bool is_white_piece;
    bool direct_to_error;
    char board[64];
    char piece;
    bool can_castle;
    enum states state_val;
    bool black_kingside;
    bool black_queenside;
    bool white_kingside;
    bool white_queenside;
    bool en_passant;
    uint8_t en_passant_square;
    uint8_t ply_since_ponr;
    uint8_t ply;
    uint16_t turn;

    uint16_t y;
    uint16_t x;

    bool is_started;
    enum move_type movetype;
} state;

void send_move(struct chess_state *st)
{
    shell_obj = (struct smak_json_obj) {
        .ply      = st->ply,
        .from     = st->x,
        .to       = st->pin_number,
        .piece    = st->board[st->pin_number],
        .captured = p_EMPTY_SQUARE,
        .movetype = (enum move_type)NORMAL,
    };
}

void send_move_castle_kingside(struct chess_state *st, bool is_white)
{

    shell_obj = (struct smak_json_obj) {
        .ply      = st->ply,
        .from     = is_white ? WHITE_KING_STARTINGSQUARE : BLACK_KING_STARTINGSQUARE,
        .to       = is_white ? WHITE_ROOK_KINGSIDE_STARTINGSQUARE
                             : BLACK_ROOK_KINGSIDE_CASTLESQUARE,
        .piece    = is_white ? p_WHITE_KING : p_BLACK_KING,
        .captured = is_white ? p_WHITE_ROOK : p_BLACK_ROOK,
        .movetype = (enum move_type)CASTLING,
    };
}

void send_move_castle_queenside(struct chess_state *st, bool is_white)
{

    shell_obj = (struct smak_json_obj) {
        .ply      = st->ply,
        .from     = is_white ? WHITE_KING_STARTINGSQUARE : BLACK_KING_STARTINGSQUARE,
        .to       = is_white ? WHITE_ROOK_QUEENSIDE_STARTINGSQUARE
                             : BLACK_ROOK_QUEENSIDE_CASTLESQUARE,
        .piece    = is_white ? p_WHITE_KING : p_BLACK_KING,
        .captured = is_white ? p_WHITE_ROOK : p_BLACK_ROOK,
        .movetype = (enum move_type)CASTLING,
    };
}

sys_slist_t move_check_list = SYS_SLIST_STATIC_INIT(&move_check_list);

static void pin_change(struct chess_state *st, const uint8_t pin_number,
                       const bool is_up);

void general_entry(void *obj);

#define DECLARE_STATE_FUNCS(name)                        \
    enum smf_state_result CONCAT(name, _run)(void *obj); \
    // void CONCAT(name, _exit)(void *obj);

/* clang-format off */

FOR_EACH(DECLARE_STATE_FUNCS, (),
    white,
    white_move,
    white_capture,
    white_enemy_capture,
    white_castling,
    white_castling_kingside_kingdown,
    white_castling_queenside_kingdown,
    white_castling_kingside_KINGUP_ROOKUP,
    white_castling_queenside_KINGUP_ROOKUP,
    white_castling_kingside_kingdown_ROOKUP,
    white_castling_queenside_kingdown_ROOKUP,
    white_castling_kingside_KINGUP_rookdown,
    white_castling_queenside_KINGUP_rookdown,
    black,
    black_move,
    black_capture,
    black_enemy_capture,
    black_castling,
    black_castling_kingside_kingdown,
    black_castling_queenside_kingdown,
    black_castling_kingside_KINGUP_ROOKUP,
    black_castling_queenside_KINGUP_ROOKUP,
    black_castling_kingside_kingdown_ROOKUP,
    black_castling_queenside_kingdown_ROOKUP,
    black_castling_kingside_KINGUP_rookdown,
    black_castling_queenside_KINGUP_rookdown,
    error);
/* clang-format on */

#define CREATE_STATE_FROM_NAME(name)                                               \
    [name] = SMF_CREATE_STATE(general_entry, CONCAT(name, _run), NULL, NULL, NULL)

#define GET_ENTRY_FUNC_FROM_NAME(name) general_entry
#define GET_RUN_FUNC_FROM_NAME(name) CONCAT(name, _run)
#define GET_EXIT_FUNC_FROM_NAME(name) CONCAT(name, _exit)

static const struct smf_state smak_states[] = {

    FOR_EACH(
        CREATE_STATE_FROM_NAME, (, ), white, white_move, white_capture,
        white_enemy_capture, white_castling, white_castling_kingside_kingdown,
        white_castling_queenside_kingdown,
        white_castling_kingside_KINGUP_ROOKUP,
        white_castling_queenside_KINGUP_ROOKUP,
        white_castling_kingside_kingdown_ROOKUP,
        white_castling_queenside_kingdown_ROOKUP,
        white_castling_kingside_KINGUP_rookdown,
        white_castling_queenside_KINGUP_rookdown, black, black_move,
        black_capture, black_enemy_capture, black_castling,
        black_castling_kingside_kingdown, black_castling_queenside_kingdown,
        black_castling_kingside_KINGUP_ROOKUP,
        black_castling_queenside_KINGUP_ROOKUP,
        black_castling_kingside_kingdown_ROOKUP,
        black_castling_queenside_kingdown_ROOKUP,
        black_castling_kingside_KINGUP_rookdown,
        black_castling_queenside_KINGUP_rookdown, error)
};

static bool can_castle(struct chess_state *st, enum states s,
                       const int pin_number);

static void dump_state(const struct chess_state *st);

void general_entry(void *obj)
{
    struct chess_state *st                          = (struct chess_state *)obj;
    __attribute__((unused)) struct smf_ctx *ctx_ptr = SMF_CTX(obj);

    if (st->pin_number > ARRAY_SIZE(state.board)) {
        st->direct_to_error = true;
        return;
    }
    // dump_state(st);
}

inline static void check_en_passant(struct chess_state *st,
                                    const int pin_number, char piece)
{
    switch (piece) {
    case p_WHITE_PAWN: {
        if (st->board[pin_number] == p_WHITE_PAWN) {
            st->en_passant        = pin_number - st->x == -16;
            st->en_passant_square = pin_number + 8;
        }
        break;
    }
    case p_BLACK_PAWN: {
        if (st->board[pin_number] == p_BLACK_PAWN) {
            st->en_passant        = pin_number - st->x == 16;
            st->en_passant_square = pin_number - 8;
        }
        break;
    }
    default:
        break;
    }
}

inline static void handle_ponr(struct chess_state *st, const int pin_number,
                               char piece)
{
    if (st->board[pin_number] == piece) {
        st->ply_since_ponr = 0;
        return;
    } else {
        st->ply_since_ponr++;
    }
}

inline static void check_castling(struct chess_state *st, const int pin_number,
                                  char piece)
{
    switch (piece) {
    case p_WHITE_KING: {
        if (st->board[pin_number] == p_WHITE_KING) {
            st->white_kingside  = false;
            st->white_queenside = false;
        } else if (st->board[pin_number] == p_WHITE_ROOK) {
            st->white_kingside  = st->white_kingside && st->x == WHITE_ROOK_KINGSIDE_STARTINGSQUARE;
            st->white_queenside = st->white_queenside && st->x == WHITE_ROOK_QUEENSIDE_STARTINGSQUARE;
        }
        break;
    }
    case p_BLACK_KING: {
        if (st->board[pin_number] == p_BLACK_KING) {
            st->black_kingside  = false;
            st->black_queenside = false;
        } else if (st->board[pin_number] == p_BLACK_ROOK) {
            st->black_kingside  = st->black_kingside && st->x == BLACK_ROOK_KINGSIDE_STARTINGSQUARE;
            st->black_queenside = st->black_queenside && st->x == BLACK_ROOK_QUEENSIDE_STARTINGSQUARE;
        }
        break;
    }
    default:
        break;
    }
}

inline static bool handle_promotion(struct chess_state *st,
                                    const int pin_number, char piece)
{
    switch (piece) {
    case p_WHITE_PAWN: {
        if (st->board[pin_number] == p_WHITE_PAWN && pin_number < 8) {
            st->board[pin_number] = p_WHITE_QUEEN;
            return true;
        } else {
            return false;
        }
    }
    case p_BLACK_PAWN: {
        if (st->board[pin_number] == p_BLACK_PAWN && pin_number > (63 - 8)) {
            st->board[pin_number] = p_BLACK_QUEEN;
            return true;
        } else {
            return false;
        }
    }
    default:
        return false;
    }
}

inline static bool can_castle(struct chess_state *st, enum states s,
                              const int pin_number)
{
    if (s == white) {
        return (st->board[pin_number] == p_WHITE_KING && (st->white_kingside || st->white_queenside));
    } else if (s == black) {
        return (st->board[pin_number] == p_BLACK_KING && (st->black_kingside || st->black_queenside));
    }

    return false;
}
#ifndef STATE
#define STATE(s) &smak_states[s]
#else
#error "macro conflict with STATE(s)"
#endif

#ifndef HANDLE_BLACK_PROMOTION
#define HANDLE_BLACK_PROMOTION(st, pin_number)     \
    handle_promotion(st, pin_number, p_BLACK_PAWN)
#endif
#ifndef HANDLE_WHITE_PROMOTION
#define HANDLE_WHITE_PROMOTION(st, pin_number)     \
    handle_promotion(st, pin_number, p_WHITE_PAWN)
#endif
#ifndef CHECK_BLACK_CASTLING
#define CHECK_BLACK_CASTLING(st, pin_number)     \
    check_castling(st, pin_number, p_BLACK_KING)
#endif
#ifndef CHECK_WHITE_CASTLING
#define CHECK_WHITE_CASTLING(st, pin_number)     \
    check_castling(st, pin_number, p_WHITE_KING)
#endif
#ifndef CHECK_BLACK_EN_PASSANT
#define CHECK_BLACK_EN_PASSANT(st, pin_number)     \
    check_en_passant(st, pin_number, p_BLACK_PAWN)
#endif
#ifndef CHECK_WHITE_EN_PASSANT
#define CHECK_WHITE_EN_PASSANT(st, pin_number)     \
    check_en_passant(st, pin_number, p_WHITE_PAWN)
#endif
#ifndef HANDLE_BLACK_PONR
#define HANDLE_BLACK_PONR(st, pin_number)     \
    handle_ponr(st, pin_number, p_BLACK_PAWN)
#endif
#ifndef HANDLE_WHITE_PONR
#define HANDLE_WHITE_PONR(st, pin_number)     \
    handle_ponr(st, pin_number, p_WHITE_PAWN)
#endif

enum smf_state_result white_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->direct_to_error) {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
        return SMF_EVENT_HANDLED;
    }
    if (st->is_down || st->is_empty_square) {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
        return SMF_EVENT_HANDLED;
    } else if (can_castle(st, white, st->pin_number)) {
        st->state_val = white_castling;
        smf_set_state(ctx_ptr, STATE(white_castling));
        return SMF_EVENT_HANDLED;
    } else if (st->is_white_piece) {
        st->state_val = white_move;
        st->x         = st->pin_number;
        smf_set_state(ctx_ptr, STATE(white_move));
        return SMF_EVENT_HANDLED;
    } else {
        st->state_val = white_enemy_capture;
        st->y         = st->pin_number;
        smf_set_state(ctx_ptr, STATE(white_enemy_capture));
        return SMF_EVENT_HANDLED;
    }
}

enum smf_state_result white_move_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    bool is_x_down = st->is_down && st->x == st->pin_number;

    if (is_x_down) {
        st->state_val = white;
        smf_set_state(ctx_ptr, STATE(white));
        return SMF_EVENT_HANDLED;
    } else if (st->is_down) {
        st->state_val = black;
        smf_set_state(ctx_ptr, STATE(black));
        st->board[st->pin_number] = st->board[st->x];
        st->board[st->x]          = p_EMPTY_SQUARE;
        st->ply++;

        CHECK_WHITE_EN_PASSANT(st, st->pin_number);
        HANDLE_WHITE_PONR(st, st->pin_number);
        CHECK_WHITE_CASTLING(st, st->pin_number);
        HANDLE_WHITE_PROMOTION(st, st->pin_number);

        send_move(st);
    } else if (st->is_black_piece) {
        st->state_val = white_capture;
        st->y         = st->pin_number;
        smf_set_state(ctx_ptr, STATE(white_capture));
    } else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result white_capture_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    bool is_y_down     = (st->is_down && (st->y == st->pin_number));
    bool is_en_passant = (st->is_down && st->en_passant && st->en_passant_square == st->pin_number);

    if (is_y_down || is_en_passant) {
        st->state_val = black;
        smf_set_state(ctx_ptr, STATE(black));
        [[maybe_unused]] const char captured = st->board[st->y];
        st->board[st->pin_number]            = st->board[st->x];
        st->board[st->x]                     = p_EMPTY_SQUARE;
        if (is_en_passant) {
            st->board[st->y] = p_EMPTY_SQUARE;
        }
        st->ply_since_ponr = 0;
        st->ply++;
        st->en_passant = false;

        CHECK_WHITE_CASTLING(st, st->pin_number);
        [[maybe_unused]] const bool promotion = HANDLE_WHITE_PROMOTION(st, st->pin_number);
        send_move(st);
    } else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(black));
    }
    return SMF_EVENT_HANDLED;
}

enum smf_state_result white_enemy_capture_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->is_up && st->is_white_piece) {
        st->state_val = white_capture;
        st->x         = st->pin_number;
        smf_set_state(ctx_ptr, STATE(white_capture));
    } else if (st->is_up) {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
        // send error
    } else if (st->pin_number == st->y) {
        st->state_val = white;
        smf_set_state(ctx_ptr, STATE(white));
    } else if (st->is_down) {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
        // send error
    }
    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;
    if (st->direct_to_error) {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
        return SMF_EVENT_HANDLED;
    }
    if (st->is_down || st->is_empty_square) {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
        return SMF_EVENT_HANDLED;
    } else if (/*st->can_castle*/ can_castle(st, black, st->pin_number)) {
        st->state_val = black_castling;
        smf_set_state(ctx_ptr, STATE(black_castling));
        return SMF_EVENT_HANDLED;
    } else if (st->is_black_piece) {
        st->state_val = black_move;
        st->x         = st->pin_number;
        smf_set_state(ctx_ptr, STATE(black_move));
        return SMF_EVENT_HANDLED;
    } else {
        st->state_val = black_enemy_capture;
        st->y         = st->pin_number;
        smf_set_state(ctx_ptr, STATE(black_enemy_capture));
        return SMF_EVENT_HANDLED;
    }
}

enum smf_state_result black_move_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    bool is_x_down = st->is_down && st->x == st->pin_number;
    if (is_x_down) {
        st->state_val = black;
        smf_set_state(ctx_ptr, STATE(black));
        return SMF_EVENT_HANDLED;
    } else if (st->is_down) {
        st->state_val = white;
        smf_set_state(ctx_ptr, STATE(white));
        st->board[st->pin_number] = st->board[st->x];
        st->board[st->x]          = p_EMPTY_SQUARE;
        st->ply++;

        CHECK_BLACK_EN_PASSANT(st, st->pin_number);
        HANDLE_BLACK_PONR(st, st->pin_number);
        CHECK_BLACK_CASTLING(st, st->pin_number);
        [[maybe_unused]] const bool promotion = HANDLE_BLACK_PROMOTION(st, st->pin_number);

        send_move(st);
    } else if (st->is_white_piece) {
        st->state_val = black_capture;
        smf_set_state(ctx_ptr, STATE(black_capture));
        st->y = st->pin_number;
    } else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }
    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_capture_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    bool is_y_down     = (st->is_down && (st->y == st->pin_number));
    bool is_en_passant = (st->is_down && st->en_passant && st->en_passant_square == st->pin_number);
    if (is_y_down || is_en_passant) {
        st->state_val = white;
        smf_set_state(ctx_ptr, STATE(white));
        [[maybe_unused]] const char captured = st->board[st->y];
        st->board[st->pin_number]            = st->board[st->x];
        st->board[st->x]                     = p_EMPTY_SQUARE;
        if (is_en_passant) {
            st->board[st->y] = p_EMPTY_SQUARE;
        }
        st->ply_since_ponr = 0;
        st->ply++;
        st->en_passant = false;

        CHECK_BLACK_CASTLING(st, st->pin_number);
        [[maybe_unused]] const bool promotion = HANDLE_BLACK_PROMOTION(st, st->pin_number);
        send_move(st);
    } else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_enemy_capture_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->is_up && st->is_black_piece) {
        st->state_val = black_capture;
        smf_set_state(ctx_ptr, STATE(black_capture));
        st->x = st->pin_number;
    } else if (st->is_up) {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    } else if (st->pin_number == st->y) {
        st->state_val = black;
        smf_set_state(ctx_ptr, STATE(black));
    } else if (st->is_down) {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result white_castling_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->is_down) {
        if (st->pin_number == WHITE_KING_STARTINGSQUARE) {
            st->state_val = white;
            smf_set_state(ctx_ptr, STATE(white));
        } else if (st->pin_number == WHITE_KING_KINGSIDE_CASTLESQUARE && st->white_kingside) {
            st->state_val = white_castling_kingside_kingdown;
            smf_set_state(ctx_ptr, STATE(white_castling_kingside_kingdown));
        } else if (st->pin_number == WHITE_KING_QUEENSIDE_CASTLESQUARE && st->white_queenside) {
            st->state_val = white_castling_queenside_kingdown;
            smf_set_state(ctx_ptr, STATE(white_castling_queenside_kingdown));
        } else {
            st->state_val = white_move;
            st->x         = WHITE_KING_STARTINGSQUARE;
            smf_set_state(ctx_ptr, STATE(white_move));
            pin_change(st, st->pin_number, st->is_up);
        }
    } else if (st->is_white_piece) {
        if (st->board[st->pin_number] == p_WHITE_ROOK) {
            if (st->pin_number == WHITE_ROOK_KINGSIDE_STARTINGSQUARE && st->white_kingside) {
                st->state_val = white_castling_kingside_KINGUP_ROOKUP;
                smf_set_state(ctx_ptr, STATE(white_castling_kingside_KINGUP_ROOKUP));
            } else if (st->pin_number == WHITE_ROOK_QUEENSIDE_STARTINGSQUARE && st->white_queenside) {
                st->state_val = white_castling_queenside_KINGUP_ROOKUP;
                smf_set_state(ctx_ptr, STATE(white_castling_queenside_KINGUP_ROOKUP));
            } else {
                st->state_val = error;
                smf_set_state(ctx_ptr, STATE(error));
            }
        } else {
            st->state_val = error;
            smf_set_state(ctx_ptr, STATE(error));
        }
    } else if (st->is_black_piece) {
        st->state_val = white_capture;
        smf_set_state(ctx_ptr, STATE(white_capture));
        st->y = st->pin_number;
        st->x = WHITE_KING_STARTINGSQUARE;
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result white_castling_kingside_kingdown_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->is_up && st->board[st->pin_number] == p_WHITE_ROOK && st->pin_number == WHITE_ROOK_KINGSIDE_STARTINGSQUARE) {
        st->state_val = white_castling_kingside_kingdown_ROOKUP;
        smf_set_state(ctx_ptr, STATE(white_castling_kingside_kingdown_ROOKUP));
    } else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result white_castling_queenside_kingdown_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->is_up && st->board[st->pin_number] == p_WHITE_ROOK && st->pin_number == WHITE_ROOK_QUEENSIDE_STARTINGSQUARE) {
        st->state_val = white_castling_queenside_kingdown_ROOKUP;
        smf_set_state(ctx_ptr, STATE(white_castling_queenside_kingdown_ROOKUP));
    } else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }
    return SMF_EVENT_HANDLED;
}

#define END_KINGSIDE_CASTLING_WHITE(st)                             \
    st->white_kingside  = false;                                    \
    st->white_queenside = false;                                    \
    st->ply++;                                                      \
    st->en_passant                                = false;          \
    st->board[WHITE_ROOK_KINGSIDE_STARTINGSQUARE] = p_EMPTY_SQUARE; \
    st->board[WHITE_ROOK_KINGSIDE_CASTLESQUARE]   = p_WHITE_ROOK;   \
    st->board[WHITE_KING_STARTINGSQUARE]          = p_EMPTY_SQUARE; \
    st->board[WHITE_KING_KINGSIDE_CASTLESQUARE]   = p_WHITE_KING;

#define END_QUEENSIDE_CASTLING_WHITE(st)                             \
    st->white_queenside = false;                                     \
    st->white_kingside  = false;                                     \
    st->ply++;                                                       \
    st->en_passant                                 = false;          \
    st->board[WHITE_ROOK_QUEENSIDE_STARTINGSQUARE] = p_EMPTY_SQUARE; \
    st->board[WHITE_ROOK_QUEENSIDE_CASTLESQUARE]   = p_WHITE_ROOK;   \
    st->board[WHITE_KING_STARTINGSQUARE]           = p_EMPTY_SQUARE; \
    st->board[WHITE_KING_QUEENSIDE_CASTLESQUARE]   = p_WHITE_KING;

enum smf_state_result white_castling_kingside_kingdown_ROOKUP_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == WHITE_ROOK_KINGSIDE_CASTLESQUARE) {
        END_KINGSIDE_CASTLING_WHITE(st);
        st->state_val = black;
        smf_set_state(ctx_ptr, STATE(black));
        send_move_castle_kingside(st, true);
    } else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result white_castling_queenside_kingdown_ROOKUP_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == WHITE_ROOK_QUEENSIDE_CASTLESQUARE) {
        END_QUEENSIDE_CASTLING_WHITE(st);

        st->state_val = black;
        smf_set_state(ctx_ptr, STATE(black));
        send_move_castle_queenside(st, true);
    } else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result white_castling_kingside_KINGUP_ROOKUP_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == WHITE_KING_KINGSIDE_CASTLESQUARE) {
        st->state_val = white_castling_kingside_kingdown_ROOKUP;
        smf_set_state(ctx_ptr, STATE(white_castling_kingside_kingdown_ROOKUP));
    } else if (st->is_down && st->pin_number == WHITE_ROOK_KINGSIDE_CASTLESQUARE) {
        st->state_val = white_castling_kingside_KINGUP_rookdown;
        smf_set_state(ctx_ptr, STATE(white_castling_kingside_KINGUP_rookdown));
    } else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result white_castling_queenside_KINGUP_ROOKUP_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == WHITE_KING_QUEENSIDE_CASTLESQUARE) {
        st->state_val = white_castling_queenside_kingdown_ROOKUP;
        smf_set_state(ctx_ptr, STATE(white_castling_queenside_kingdown_ROOKUP));
    } else if (st->is_down && st->pin_number == WHITE_ROOK_QUEENSIDE_CASTLESQUARE) {
        st->state_val = white_castling_queenside_KINGUP_rookdown;
        smf_set_state(ctx_ptr, STATE(white_castling_queenside_KINGUP_rookdown));
    } else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result white_castling_kingside_KINGUP_rookdown_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == WHITE_KING_KINGSIDE_CASTLESQUARE) {
        END_KINGSIDE_CASTLING_WHITE(st);
        st->state_val = black;
        smf_set_state(ctx_ptr, STATE(black));

        send_move_castle_kingside(st, true);
    } else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result white_castling_queenside_KINGUP_rookdown_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == WHITE_KING_QUEENSIDE_CASTLESQUARE) {
        END_QUEENSIDE_CASTLING_WHITE(st);
        st->state_val = black;
        smf_set_state(ctx_ptr, STATE(black));

        send_move_castle_queenside(st, true);
    } else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

#define END_KINGSIDE_CASTLING_BLACK(st)                             \
    st->black_kingside  = false;                                    \
    st->black_queenside = false;                                    \
    st->ply++;                                                      \
    st->en_passant                                = false;          \
    st->board[BLACK_ROOK_KINGSIDE_STARTINGSQUARE] = p_EMPTY_SQUARE; \
    st->board[BLACK_ROOK_KINGSIDE_CASTLESQUARE]   = p_BLACK_ROOK;   \
    st->board[BLACK_KING_STARTINGSQUARE]          = p_EMPTY_SQUARE; \
    st->board[BLACK_KING_KINGSIDE_CASTLESQUARE]   = p_BLACK_KING;

#define END_QUEENSIDE_CASTLING_BLACK(st)                             \
    st->black_queenside = false;                                     \
    st->black_kingside  = false;                                     \
    st->ply++;                                                       \
    st->en_passant                                 = false;          \
    st->board[BLACK_ROOK_QUEENSIDE_STARTINGSQUARE] = p_EMPTY_SQUARE; \
    st->board[BLACK_ROOK_QUEENSIDE_CASTLESQUARE]   = p_BLACK_ROOK;   \
    st->board[BLACK_KING_STARTINGSQUARE]           = p_EMPTY_SQUARE; \
    st->board[BLACK_KING_QUEENSIDE_CASTLESQUARE]   = p_BLACK_KING;

enum smf_state_result black_castling_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->is_down) {
        if (st->pin_number == BLACK_KING_STARTINGSQUARE) {
            st->state_val = black;
            smf_set_state(ctx_ptr, STATE(black));
        } else if (st->pin_number == BLACK_KING_KINGSIDE_CASTLESQUARE && st->black_kingside) {
            st->state_val = black_castling_kingside_kingdown;
            smf_set_state(ctx_ptr, STATE(black_castling_kingside_kingdown));
        } else if (st->pin_number == BLACK_KING_QUEENSIDE_CASTLESQUARE && st->black_queenside) {
            st->state_val = black_castling_queenside_kingdown;
            smf_set_state(ctx_ptr, STATE(black_castling_queenside_kingdown));
        } else {
            st->state_val = black_move;
            st->x         = BLACK_KING_STARTINGSQUARE;
            smf_set_state(ctx_ptr, STATE(black_move));
            pin_change(st, st->pin_number, st->is_up);
        }
    } else if (st->is_black_piece) {
        if (st->board[st->pin_number] == p_BLACK_ROOK) {
            if (st->pin_number == BLACK_ROOK_KINGSIDE_STARTINGSQUARE && st->black_kingside) {
                st->state_val = black_castling_kingside_KINGUP_ROOKUP;
                smf_set_state(ctx_ptr, STATE(black_castling_kingside_KINGUP_ROOKUP));
            } else if (st->pin_number == BLACK_ROOK_QUEENSIDE_STARTINGSQUARE && st->black_queenside) {
                st->state_val = black_castling_queenside_KINGUP_ROOKUP;
                smf_set_state(ctx_ptr, STATE(black_castling_queenside_KINGUP_ROOKUP));
            } else {
                st->state_val = error;
                smf_set_state(ctx_ptr, STATE(error));
            }
        } else {
            st->state_val = error;
            smf_set_state(ctx_ptr, STATE(error));
        }
    } else if (st->is_white_piece) {
        st->state_val = black_capture;
        smf_set_state(ctx_ptr, STATE(black_capture));
        st->y = st->pin_number;
        st->x = BLACK_KING_STARTINGSQUARE;
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_castling_kingside_kingdown_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->is_up && st->board[st->pin_number] == p_BLACK_ROOK && st->pin_number == BLACK_ROOK_KINGSIDE_STARTINGSQUARE) {
        st->state_val = black_castling_kingside_kingdown_ROOKUP;
        smf_set_state(ctx_ptr, STATE(black_castling_kingside_kingdown_ROOKUP));
    } else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_castling_queenside_kingdown_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->is_up && st->board[st->pin_number] == p_BLACK_ROOK && st->pin_number == BLACK_ROOK_QUEENSIDE_STARTINGSQUARE) {
        st->state_val = black_castling_queenside_kingdown_ROOKUP;
        smf_set_state(ctx_ptr, STATE(black_castling_queenside_kingdown_ROOKUP));
    } else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_castling_kingside_kingdown_ROOKUP_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == BLACK_ROOK_KINGSIDE_CASTLESQUARE) {
        END_KINGSIDE_CASTLING_BLACK(st);
        st->state_val = white;
        smf_set_state(ctx_ptr, STATE(white));

        send_move_castle_kingside(st, false);
    } else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_castling_queenside_kingdown_ROOKUP_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == BLACK_ROOK_QUEENSIDE_CASTLESQUARE) {
        END_QUEENSIDE_CASTLING_BLACK(st);
        st->state_val = white;
        smf_set_state(ctx_ptr, STATE(white));

        send_move_castle_queenside(st, false);
    } else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_castling_kingside_KINGUP_ROOKUP_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == BLACK_KING_KINGSIDE_CASTLESQUARE) {
        st->state_val = black_castling_kingside_kingdown_ROOKUP;
        smf_set_state(ctx_ptr, STATE(black_castling_kingside_kingdown_ROOKUP));
    } else if (st->is_down && st->pin_number == BLACK_ROOK_KINGSIDE_CASTLESQUARE) {
        st->state_val = black_castling_kingside_KINGUP_rookdown;
        smf_set_state(ctx_ptr, STATE(black_castling_kingside_KINGUP_rookdown));
    } else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_castling_queenside_KINGUP_ROOKUP_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == BLACK_KING_QUEENSIDE_CASTLESQUARE) {
        st->state_val = black_castling_queenside_kingdown_ROOKUP;
        smf_set_state(ctx_ptr, STATE(black_castling_queenside_kingdown_ROOKUP));
    } else if (st->is_down && st->pin_number == BLACK_ROOK_QUEENSIDE_CASTLESQUARE) {
        st->state_val = black_castling_queenside_KINGUP_rookdown;
        smf_set_state(ctx_ptr, STATE(black_castling_queenside_KINGUP_rookdown));
    } else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_castling_kingside_KINGUP_rookdown_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == BLACK_KING_KINGSIDE_CASTLESQUARE) {
        END_KINGSIDE_CASTLING_BLACK(st);
        st->state_val = white;
        smf_set_state(ctx_ptr, STATE(white));

        send_move_castle_kingside(st, false);
    } else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_castling_queenside_KINGUP_rookdown_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state *st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == BLACK_KING_QUEENSIDE_CASTLESQUARE) {
        END_QUEENSIDE_CASTLING_BLACK(st);
        st->state_val = white;
        smf_set_state(ctx_ptr, STATE(white));
        send_move_castle_queenside(st, false);
    } else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result error_run(void *obj)
{
    printk("We died.");
    smf_set_terminate(SMF_CTX(obj), error);
    return (enum smf_state_result)error;
}

void reset_fsm(struct chess_state *st)
{
    /* probably get a mutex lock for this*/
    smf_set_terminate(SMF_CTX(st), -1);

    /* maybe wait for a semaphore or smth to see when the state has stopped
     * executing */

    for (size_t i = 0; i < ARRAY_SIZE(starting_board_initializer); i++) {
        st->board[i] = starting_board_initializer[i];
    }

    st->state_val         = white;
    st->black_kingside    = true;
    st->black_queenside   = true;
    st->white_kingside    = true;
    st->white_queenside   = true;
    st->en_passant        = false;
    st->en_passant_square = 0;
    st->ply_since_ponr    = 0;
    st->ply               = 0;
    st->x                 = 0;
    st->y                 = 0;
    st->is_down           = true;
    st->is_up             = false;
    st->is_empty_square   = false;
    st->is_white_piece    = false;
    st->is_black_piece    = false;
    st->is_started        = false;
    st->piece             = ' ';
    st->direct_to_error   = false;
    // smf_set_initial(SMF_CTX(st), STATE(white));
}

static void cleanup_linked_fsm_nodes(sys_slist_t *slist);

void clean_state(struct chess_state *st)
{
    reset_fsm(st);

    for (size_t i = 0; i < ARRAY_SIZE(empty_board_initializer); i++) {
        st->board[i] = empty_board_initializer[i];
    }

    cleanup_linked_fsm_nodes(&move_check_list);
}

static void pin_change(struct chess_state *st, const uint8_t pin_number,
                       const bool is_up)
{
    st->pin_number = pin_number;
    st->is_up      = is_up;
    st->is_down    = !is_up;

    st->piece = st->board[st->pin_number];

    st->is_empty_square = (st->piece == ' ');
    st->is_black_piece  = !(st->is_empty_square) && !(st->piece < 'a');
    st->is_white_piece  = !(st->is_empty_square) && !(st->piece > 'a');

    if (st->state_val == black || st->state_val == white) {

        st->can_castle = can_castle(st, st->state_val, st->pin_number);
    }

    if (!st->is_started) {
        st->is_started = true;
        smf_set_initial(SMF_CTX(st), STATE(white));
        smf_run_state(SMF_CTX(st));
    } else {
        smf_run_state(SMF_CTX(st));
    }
}

#ifdef CONFIG_ZTEST
static void pin_change_test(struct chess_state *st, const uint8_t pin_number,
                            const bool is_up, enum states starting_state)
{
    st->pin_number = pin_number;
    st->is_up      = is_up;
    st->is_down    = !is_up;

    st->piece = st->board[st->pin_number];

    st->is_empty_square = (st->piece == ' ');
    st->is_black_piece  = !(st->is_empty_square) && !(st->piece < 'a');
    st->is_white_piece  = !(st->is_empty_square) && !(st->piece > 'a');

    if (st->state_val == black || st->state_val == white) {

        st->can_castle = can_castle(st, st->state_val, st->pin_number);
    }

    if (!st->is_started) {
        st->is_started = true;
        smf_set_initial(SMF_CTX(st), STATE(starting_state));
        smf_run_state(SMF_CTX(st));
    } else {
        smf_run_state(SMF_CTX(st));
    }
}
#endif

[[maybe_unused]] static void dump_state(const struct chess_state *st)
{
    printk("Current state = %d\n", st->state_val);
    printk("BK = %s\n", st->black_kingside ? "true" : "false");
    printk("BQ = %s\n", st->black_queenside ? "true" : "false");
    printk("WK = %s\n", st->white_kingside ? "true" : "false");
    printk("WQ = %s\n", st->white_queenside ? "true" : "false");
    printk("EP = %s\n", st->en_passant ? "true" : "false");
    printk("PSPONR = %d\n", st->ply_since_ponr);
    printk("PLY = %d\n", st->ply);
    printk("x = %d\n", st->x);
    printk("y = %d\n", st->y);
    printk("up = %s\n", st->is_up ? "true" : "false");
    printk("down = %s\n", st->is_down ? "true" : "false");
    printk("black piece = %s\n", st->is_black_piece ? "true" : "false");
    printk("white piece = %s\n", st->is_white_piece ? "true" : "false");
}

K_FIFO_DEFINE(fsm_fifo);

struct __aligned(8) fsm_fifo_item_t {
    void *fifo_reserved;
    int pin;
    bool is_up;
    struct chess_state *st;
};

K_MUTEX_DEFINE(fsm_mutex);

void do_work_with_fifo(struct k_work *work)
{
    scope_guard(k_mutex)(&fsm_mutex);
    struct fsm_fifo_item_t *ffi = k_fifo_get(&fsm_fifo, K_FOREVER);

    pin_change(ffi->st, ffi->pin, ffi->is_up);
    free(ffi);
}

K_SEM_DEFINE(sem_fifo_access, 0, 1);
K_SEM_DEFINE(sem_list_accessible, 0, 64);

void fifo_listener_entry(void)
{
    while (1) {
        k_sem_take(&sem_fifo_access, K_FOREVER);
        struct fsm_fifo_item_t *ffi = k_fifo_get(&fsm_fifo, K_FOREVER);

        pin_change(ffi->st, ffi->pin, ffi->is_up);
        struct fsm_state_node node = {
            .state = ffi->st->state_val,
        };
        free(ffi);

        struct fsm_state_node *node_alloc = malloc(sizeof(struct fsm_state_node));

        memcpy(node_alloc, &node, sizeof(struct fsm_state_node));

        sys_slist_append(&move_check_list, &node_alloc->node);
        k_sem_give(&sem_list_accessible);
        k_sem_give(&sem_fifo_access);
    }
}

K_THREAD_DEFINE(fifo_listener, 1024, fifo_listener_entry, NULL, NULL, NULL, 7,
                0, 0);

void queue_fsm_work_fifo(struct chess_state *st, int pin, bool is_up)
{
    struct fsm_fifo_item_t item = { .pin = pin, .is_up = is_up, .st = st };

    struct fsm_fifo_item_t *mem = malloc(sizeof(struct fsm_fifo_item_t));

    memcpy(mem, &item, sizeof(struct fsm_fifo_item_t));

    scope_guard(k_mutex)(&fsm_mutex);
    k_fifo_put(&fsm_fifo, mem);
    k_sem_give(&sem_fifo_access);
}

static void cleanup_linked_fsm_nodes(sys_slist_t *list)
{
    if (list) {
        struct fsm_state_node *ptr;
        struct fsm_state_node *new_ptr;

        while (sys_slist_len(&move_check_list)) {
            ptr     = SYS_SLIST_PEEK_HEAD_CONTAINER(&move_check_list, ptr, node);
            new_ptr = CONTAINER_OF(sys_slist_get(&move_check_list),
                                   struct fsm_state_node, node);
            free(ptr);
        }

        while (!k_sem_take(&sem_list_accessible, K_NO_WAIT)) {
        };
    }
}
SCOPE_DEFER_DEFINE(cleanup_linked_fsm_nodes, sys_slist_t *);

sys_slist_t *move_check_list_get(void) { return &move_check_list; }
struct chess_state *state_get(void) { return &state; }
const char *state_board_get(struct chess_state *st) { return st->board; }

[[gnu::constructor]] void startup_fsm(void) { reset_fsm(&state); }

#ifdef CONFIG_ZTEST
#include "fsm_tests.c"
#else
#undef STATE
#endif