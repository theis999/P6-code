#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/smf.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test.h>

static const char p_EMPTY_SQUARE = ' ';
static const char p_BLACK_PAWN = 'p', p_WHITE_PAWN = 'P';
static const char p_BLACK_ROOK = 'r', p_WHITE_ROOK = 'R';
static const char p_BLACK_KNIGHT = 'n', p_WHITE_KNIGHT = 'N';
static const char p_BLACK_BISHOP = 'b', p_WHITE_BISHOP = 'B';
static const char p_BLACK_QUEEN = 'q', p_WHITE_QUEEN = 'Q';
static const char p_BLACK_KING = 'k', p_WHITE_KING = 'K';

static const int WHITE_KING_STARTINGSQUARE = 63 - 3;
static const int BLACK_KING_STARTINGSQUARE = 0 + 4;

static const int WHITE_ROOK_KINGSIDE_STARTINGSQUARE = 63 - 0;
static const int WHITE_ROOK_QUEENSIDE_STARTINGSQUARE = 63 - 7;
static const int BLACK_ROOK_KINGSIDE_STARTINGSQUARE = 0 + 7;
static const int BLACK_ROOK_QUEENSIDE_STARTINGSQUARE = 0;

static const int WHITE_KING_KINGSIDE_CASTLESQUARE = 63 - 1;
static const int WHITE_KING_QUEENSIDE_CASTLESQUARE = 63 - 5;
static const int BLACK_KING_KINGSIDE_CASTLESQUARE = 0 + 6;
static const int BLACK_KING_QUEENSIDE_CASTLESQUARE = 0 + 2;

static const int WHITE_ROOK_KINGSIDE_CASTLESQUARE = 63 - 2;
static const int WHITE_ROOK_QUEENSIDE_CASTLESQUARE = 63 - 4;
static const int BLACK_ROOK_KINGSIDE_CASTLESQUARE = 0 + 5;
static const int BLACK_ROOK_QUEENSIDE_CASTLESQUARE = 0 + 3;

static char board[64] = 
{
	'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r',
	'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P',
	'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R',
};

static const char starting_board_initializer[64] = {
	'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r',
	'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P',
	'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R',
};

static const char empty_board_initializer[64] = {
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
	' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
};


enum states {
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

	error,
};

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
} state;

static uint8_t pin(const char *restrict str) {
    char file_char = *str;
    char rank_char = *(str+1);

    uint8_t file = (file_char - 'a');
    uint8_t rank = (rank_char - '0');
    uint8_t r = (8 - rank) << 3;

    return (r | file);
}

void general_entry(void *obj);

#define DECLARE_STATE_FUNCS(name) \
enum smf_state_result CONCAT(name, _run)(void *obj);\
//void CONCAT(name, _exit)(void *obj);

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
	error
);

// #define CREATE_STATE_FROM_NAME(name) [name] = SMF_CREATE_STATE(general_entry, CONCAT(name, _run), CONCAT(name, _exit), NULL, NULL)

#define CREATE_STATE_FROM_NAME(name) [name] = SMF_CREATE_STATE(general_entry, CONCAT(name, _run), NULL, NULL, NULL)

#define GET_ENTRY_FUNC_FROM_NAME(name) general_entry
#define GET_RUN_FUNC_FROM_NAME(name) CONCAT(name, _run)
#define GET_EXIT_FUNC_FROM_NAME(name) CONCAT(name, _exit)

static const struct smf_state smak_states[] = {

    FOR_EACH(CREATE_STATE_FROM_NAME, (,),     
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
        error
    )
};

static bool can_castle(struct chess_state *st, enum states s, const int pin_number);

static void dump_state(const struct chess_state *st);


void general_entry(void *obj) 
{
    struct chess_state *st  = (struct chess_state *)obj;
    struct smf_ctx *ctx_ptr = SMF_CTX(obj); 
    
    if (st->pin_number > ARRAY_SIZE(state.board)) {
        st->direct_to_error = true;
        return;
    }
    //dump_state(st);
}

inline static void check_en_passant(struct chess_state *st, const int pin_number, char piece) 
{
    switch (piece) {
    case p_WHITE_PAWN: {
        if (st->board[pin_number] == p_WHITE_PAWN) {
            st->en_passant          = pin_number - st->x == -16;
            st->en_passant_square   = pin_number + 8;
        }
        break;
    }
    case p_BLACK_PAWN: {
        if (st->board[pin_number] == p_BLACK_PAWN) {
            st->en_passant          = pin_number - st->x == 16;
            st->en_passant_square   = pin_number - 8;
        }
        break;
    }
    default: 
        break;
    }
} 

inline static void handle_ponr(struct chess_state *st, const int pin_number, char piece) 
{
    if (st->board[pin_number] == piece) {
        st->ply_since_ponr = 0;
        return;
    } 
    else {
        st->ply_since_ponr++;
    }
}

inline static void check_castling(struct chess_state *st, const int pin_number, char piece)
{
    switch (piece) {
    case p_WHITE_KING: {
        if (st->board[pin_number] == p_WHITE_KING) {
            st->white_kingside  = false;
            st->white_queenside = false;
        }
        else if (st->board[pin_number] == p_WHITE_ROOK) {
            st->white_kingside  = st->white_kingside  && st->x == WHITE_ROOK_KINGSIDE_STARTINGSQUARE;
            st->white_queenside = st->white_queenside && st->x == WHITE_ROOK_QUEENSIDE_STARTINGSQUARE;
        }
        break;
    }
    case p_BLACK_KING: {
        if (st->board[pin_number] == p_BLACK_KING) {
            st->black_kingside  = false;
            st->black_queenside = false;
        }
        else if (st->board[pin_number] == p_BLACK_ROOK) {
            st->black_kingside  = st->black_kingside  && st->x == BLACK_ROOK_KINGSIDE_STARTINGSQUARE;
            st->black_queenside = st->black_queenside && st->x == BLACK_ROOK_QUEENSIDE_STARTINGSQUARE;
        }
        break;
    }
    default: 
        break;
    }
}

inline static bool handle_promotion(struct chess_state *st, const int pin_number, char piece)
{
    switch (piece) {
    case p_WHITE_PAWN: {
        if (st->board[pin_number] == p_WHITE_PAWN && pin_number < 8) {
            st->board[pin_number] = p_WHITE_QUEEN;
            return true;
        } 
        else {
            return false;
        }
    }
    case p_BLACK_PAWN: {
        if (st->board[pin_number] == p_BLACK_PAWN && pin_number > (63-8)) {
            st->board[pin_number] = p_BLACK_QUEEN;
            return true;
        }
        else {
            return false;
        }
    }
    default:
        return false;
    }
}

inline static bool can_castle (struct chess_state *st, enum states s, const int pin_number) 
{
    if (s == white) {
        return (st->board[pin_number] == p_WHITE_KING && (st->white_kingside || st->white_queenside));
    } 
    else if (s == black) {
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
#define HANDLE_BLACK_PROMOTION(st, pin_number) handle_promotion(st, pin_number, p_BLACK_PAWN)
#endif
#ifndef HANDLE_WHITE_PROMOTION 
#define HANDLE_WHITE_PROMOTION(st, pin_number) handle_promotion(st, pin_number, p_WHITE_PAWN)
#endif
#ifndef CHECK_BLACK_CASTLING
#define CHECK_BLACK_CASTLING(st, pin_number) check_castling(st, pin_number, p_BLACK_KING)
#endif
#ifndef CHECK_WHITE_CASTLING
#define CHECK_WHITE_CASTLING(st, pin_number) check_castling(st, pin_number, p_WHITE_KING)
#endif
#ifndef CHECK_BLACK_EN_PASSANT
#define CHECK_BLACK_EN_PASSANT(st, pin_number) check_en_passant(st, pin_number, p_BLACK_PAWN)
#endif
#ifndef CHECK_WHITE_EN_PASSANT
#define CHECK_WHITE_EN_PASSANT(st, pin_number) check_en_passant(st, pin_number, p_WHITE_PAWN)
#endif
#ifndef HANDLE_BLACK_PONR
#define HANDLE_BLACK_PONR(st, pin_number) handle_ponr(st, pin_number, p_BLACK_PAWN)
#endif
#ifndef HANDLE_WHITE_PONR
#define HANDLE_WHITE_PONR(st, pin_number) handle_ponr(st, pin_number, p_WHITE_PAWN)
#endif


enum smf_state_result white_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;

    if (st->direct_to_error) {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
        return SMF_EVENT_HANDLED;
    } 
    if (st->is_down || st->is_empty_square) {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
        return SMF_EVENT_HANDLED;
    }
    else if (can_castle(st, white, st->pin_number)) {
        st->state_val = white_castling;
        smf_set_state(ctx_ptr, STATE(white_castling));
        return SMF_EVENT_HANDLED;
    }
    else if (st->is_white_piece) {
        st->state_val = white_move;
        st->x = st->pin_number;
        smf_set_state(ctx_ptr, STATE(white_move));
        return SMF_EVENT_HANDLED;
    }
    else {
        st->state_val = white_enemy_capture;
        st->y = st->pin_number;
        smf_set_state(ctx_ptr, STATE(white_enemy_capture));
        return SMF_EVENT_HANDLED;
    }
}

enum smf_state_result white_move_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;

    bool is_x_down = st->is_down && st->x == st->pin_number;
    
    if (is_x_down) {
        st->state_val = white;
        smf_set_state(ctx_ptr, STATE(white));
        return SMF_EVENT_HANDLED;
    }
    else if (st->is_down) {
        st->state_val = black;
        smf_set_state(ctx_ptr, STATE(black));
        st->board[st->pin_number] = st->board[st->x];
        st->board[st->x] = p_EMPTY_SQUARE;
        st->ply++;

        CHECK_WHITE_EN_PASSANT(st, st->pin_number);
        HANDLE_WHITE_PONR(st, st->pin_number);
        CHECK_WHITE_CASTLING(st, st->pin_number);
        HANDLE_WHITE_PROMOTION(st, st->pin_number);
    }
    else if (st->is_black_piece) {
        st->state_val = white_capture;
        st->y = st->pin_number;
        smf_set_state(ctx_ptr, STATE(white_capture));
    } 
    else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result white_capture_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;

    bool is_y_down      = (st->is_down && (st->y == st->pin_number));
    bool is_en_passant  = (st->is_down && st->en_passant && st->en_passant_square == st->pin_number);

    if (is_y_down || is_en_passant) {
        st->state_val = black;
        smf_set_state(ctx_ptr, STATE(black));
        const char captured = st->board[st->y];
        st->board[st->pin_number] = st->board[st->x];
        st->board[st->x] = p_EMPTY_SQUARE;
        if (is_en_passant) {
            st->board[st->y] = p_EMPTY_SQUARE;
        }
        st->ply_since_ponr = 0;
        st->ply++;
        st->en_passant = false;

        CHECK_WHITE_CASTLING(st, st->pin_number);
        const bool promotion = HANDLE_WHITE_PROMOTION(st, st->pin_number);
    } 
    else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(black));
    }
    return SMF_EVENT_HANDLED;
}

enum smf_state_result white_enemy_capture_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;

    if (st->is_up && st->is_white_piece) {
        st->state_val = white_capture;
        st->x = st->pin_number;
        smf_set_state(ctx_ptr, STATE(white_capture));
    }
    else if (st->is_up) {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
        // send error
    }
    else if (st->pin_number == st->y) {
        st->state_val = white;
        smf_set_state(ctx_ptr, STATE(white));
    }
    else if (st->is_down) {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
        // send error
    }
    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;
    if (st->direct_to_error) {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
        return SMF_EVENT_HANDLED;
    } 
    if (st->is_down || st->is_empty_square) {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
        return SMF_EVENT_HANDLED;
    }
    else if (/*st->can_castle*/ can_castle(st, black, st->pin_number)  ) {
        st->state_val = black_castling;
        smf_set_state(ctx_ptr, STATE(black_castling));
        return SMF_EVENT_HANDLED;
    }
    else if (st->is_black_piece) {
        st->state_val = black_move;
        st->x = st->pin_number;
        smf_set_state(ctx_ptr, STATE(black_move));
        return SMF_EVENT_HANDLED;
    }
    else {
        st->state_val = black_enemy_capture;
        st->y = st->pin_number;
        smf_set_state(ctx_ptr, STATE(black_enemy_capture));
        return SMF_EVENT_HANDLED;
    }
}

enum smf_state_result black_move_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;
    
    bool is_x_down = st->is_down && st->x == st->pin_number;
    if (is_x_down) {
        st->state_val = black;
        smf_set_state(ctx_ptr, STATE(black));
        return SMF_EVENT_HANDLED;
    }
    else if (st->is_down) {    
        st->state_val = white;
        smf_set_state(ctx_ptr, STATE(white));
        st->board[st->pin_number] = st->board[st->x];
        st->board[st->x] = p_EMPTY_SQUARE;
        st->ply++;

        CHECK_BLACK_EN_PASSANT(st, st->pin_number);
        HANDLE_BLACK_PONR(st, st->pin_number);
        CHECK_BLACK_CASTLING(st, st->pin_number);
        const bool promotion = HANDLE_BLACK_PROMOTION(st, st->pin_number);

        // SEND MOVE
    }
    else if (st->is_white_piece) {
        st->state_val = black_capture;
        smf_set_state(ctx_ptr, STATE(black_capture));
        st->y = st->pin_number;
    }
    else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }
    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_capture_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;

    bool is_y_down      = (st->is_down && (st->y == st->pin_number));
    bool is_en_passant  = (st->is_down && st->en_passant && st->en_passant_square == st->pin_number);
    if (is_y_down || is_en_passant) {
        st->state_val = white;
        smf_set_state(ctx_ptr, STATE(white));
        const char captured = st->board[st->y];
        st->board[st->pin_number] = st->board[st->x];
        st->board[st->x] = p_EMPTY_SQUARE;
        if (is_en_passant) {
            st->board[st->y] = p_EMPTY_SQUARE;
        }
        st->ply_since_ponr = 0;
        st->ply++;
        st->en_passant = false;

        CHECK_BLACK_CASTLING(st, st->pin_number);
        const bool promotion = HANDLE_BLACK_PROMOTION(st, st->pin_number);
       // send move
    } 
    else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }
   
    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_enemy_capture_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;

    if (st->is_up && st->is_black_piece) {
        st->state_val = black_capture;
        smf_set_state(ctx_ptr, STATE(black_capture));
        st->x = st->pin_number;
    }
    else if (st->is_up) {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }
    else if (st->pin_number == st->y) {
        st->state_val = black;
        smf_set_state(ctx_ptr, STATE(black));
    }
    else if (st->is_down) {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result white_castling_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;
    
    if (st->is_down) {
        if (st->pin_number == WHITE_KING_STARTINGSQUARE) {
            st->state_val = white;
            smf_set_state(ctx_ptr, STATE(white));
        }
        else if (st->pin_number == WHITE_KING_KINGSIDE_CASTLESQUARE && st->white_kingside) {
            st->state_val = white_castling_kingside_kingdown;
            smf_set_state(ctx_ptr, STATE(white_castling_kingside_kingdown));
        }
        else if (st->pin_number == WHITE_KING_QUEENSIDE_CASTLESQUARE && st->white_queenside) {
            st->state_val = white_castling_queenside_kingdown;
            smf_set_state(ctx_ptr, STATE(white_castling_queenside_kingdown));
        } else {
            st->state_val = white_move;
            st->x = WHITE_KING_STARTINGSQUARE;
            smf_set_state(ctx_ptr, STATE(white_move));
        }   
    }
    else if (st->is_white_piece) {
        if (st->board[st->pin_number] == p_WHITE_ROOK) {
            if (st->pin_number == WHITE_ROOK_KINGSIDE_STARTINGSQUARE && st->white_kingside) {
                st->state_val = white_castling_kingside_KINGUP_ROOKUP;
                smf_set_state(ctx_ptr, STATE(white_castling_kingside_KINGUP_ROOKUP));
            }
            else if (st->pin_number == WHITE_ROOK_QUEENSIDE_STARTINGSQUARE && st->white_queenside) {
                st->state_val = white_castling_queenside_KINGUP_ROOKUP;
                smf_set_state(ctx_ptr, STATE(white_castling_queenside_KINGUP_ROOKUP));
            }
            else {
                st->state_val = error;
                smf_set_state(ctx_ptr, STATE(error));    
            }
        }
        else {
            st->state_val = error;
            smf_set_state(ctx_ptr, STATE(error));
        }
    }
    else if (st->is_black_piece) {
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
    struct chess_state* st  = (struct chess_state *)obj;

    if (st->is_up && st->board[st->pin_number] == p_WHITE_ROOK && st->pin_number == WHITE_ROOK_KINGSIDE_STARTINGSQUARE) {
        st->state_val = white_castling_kingside_kingdown_ROOKUP;
        smf_set_state(ctx_ptr, STATE(white_castling_kingside_kingdown_ROOKUP));
    } 
    else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result white_castling_queenside_kingdown_run(void *obj)
{    
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;
    
    if (st->is_up && st->board[st->pin_number] == p_WHITE_ROOK && st->pin_number == WHITE_ROOK_QUEENSIDE_STARTINGSQUARE) {
        st->state_val = white_castling_queenside_kingdown_ROOKUP;
        smf_set_state(ctx_ptr, STATE(white_castling_queenside_kingdown_ROOKUP));
    }
    else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }
    return SMF_EVENT_HANDLED;
}

#define END_KINGSIDE_CASTLING_WHITE(st) \
    st->white_kingside  = false;\
    st->white_queenside = false;\
    st->ply++;\
    st->en_passant = false;\
    st->board[WHITE_ROOK_KINGSIDE_STARTINGSQUARE]   = p_EMPTY_SQUARE;\
    st->board[WHITE_ROOK_KINGSIDE_CASTLESQUARE]     = p_WHITE_ROOK;\
    st->board[WHITE_KING_STARTINGSQUARE]            = p_EMPTY_SQUARE;\
    st->board[WHITE_KING_KINGSIDE_CASTLESQUARE]     = p_WHITE_KING;

#define END_QUEENSIDE_CASTLING_WHITE(st)\
    st->white_queenside = false;\
    st->white_kingside  = false;\
    st->ply++;\
    st->en_passant = false;\
    st->board[WHITE_ROOK_QUEENSIDE_STARTINGSQUARE]  = p_EMPTY_SQUARE;\
    st->board[WHITE_ROOK_QUEENSIDE_CASTLESQUARE]    = p_WHITE_ROOK;\
    st->board[WHITE_KING_STARTINGSQUARE]            = p_EMPTY_SQUARE;\
    st->board[WHITE_KING_QUEENSIDE_CASTLESQUARE]    = p_WHITE_KING;

enum smf_state_result white_castling_kingside_kingdown_ROOKUP_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == WHITE_ROOK_KINGSIDE_CASTLESQUARE) {
        END_KINGSIDE_CASTLING_WHITE(st);
        st->state_val = black;
        smf_set_state(ctx_ptr, STATE(black));
        //send move;
    }
    else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result white_castling_queenside_kingdown_ROOKUP_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == WHITE_ROOK_QUEENSIDE_CASTLESQUARE)
    {
        END_QUEENSIDE_CASTLING_WHITE(st);

        st->state_val = black;
        smf_set_state(ctx_ptr, STATE(black));
    }
    else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result white_castling_kingside_KINGUP_ROOKUP_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == WHITE_KING_KINGSIDE_CASTLESQUARE) {
        st->state_val = white_castling_kingside_kingdown_ROOKUP;
        smf_set_state(ctx_ptr, STATE(white_castling_kingside_kingdown_ROOKUP));
    }
    else if (st->is_down && st->pin_number == WHITE_ROOK_KINGSIDE_CASTLESQUARE) {
        st->state_val = white_castling_kingside_KINGUP_rookdown;
        smf_set_state(ctx_ptr, STATE(white_castling_kingside_KINGUP_rookdown));
    } 
    else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result white_castling_queenside_KINGUP_ROOKUP_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == WHITE_KING_QUEENSIDE_CASTLESQUARE) {
        st->state_val = white_castling_queenside_kingdown_ROOKUP;
        smf_set_state(ctx_ptr, STATE(white_castling_queenside_kingdown_ROOKUP));
    }
    else if (st->is_down && st->pin_number == WHITE_ROOK_QUEENSIDE_CASTLESQUARE) {
        st->state_val = white_castling_queenside_KINGUP_rookdown;
        smf_set_state(ctx_ptr,STATE(white_castling_queenside_KINGUP_rookdown));
    }
    else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result white_castling_kingside_KINGUP_rookdown_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == WHITE_KING_KINGSIDE_CASTLESQUARE) {
        END_KINGSIDE_CASTLING_WHITE(st);
        st->state_val = black;
        smf_set_state(ctx_ptr, STATE(black));
        
        // send move
    }
    else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result white_castling_queenside_KINGUP_rookdown_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == WHITE_KING_QUEENSIDE_CASTLESQUARE) {
        END_QUEENSIDE_CASTLING_WHITE(st);
        st->state_val = black;
        smf_set_state(ctx_ptr, STATE(black));
    }
    else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

#define END_KINGSIDE_CASTLING_BLACK(st) \
    st->black_kingside  = false;\
    st->black_queenside = false;\
    st->ply++;\
    st->en_passant = false;\
    st->board[BLACK_ROOK_KINGSIDE_STARTINGSQUARE]   = p_EMPTY_SQUARE;\
    st->board[BLACK_ROOK_KINGSIDE_CASTLESQUARE]     = p_BLACK_ROOK;\
    st->board[BLACK_KING_STARTINGSQUARE]            = p_EMPTY_SQUARE;\
    st->board[BLACK_KING_KINGSIDE_CASTLESQUARE]     = p_BLACK_KING;

#define END_QUEENSIDE_CASTLING_BLACK(st)\
    st->black_queenside = false;\
    st->black_kingside  = false;\
    st->ply++;\
    st->en_passant = false;\
    st->board[BLACK_ROOK_QUEENSIDE_STARTINGSQUARE]  = p_EMPTY_SQUARE;\
    st->board[BLACK_ROOK_QUEENSIDE_CASTLESQUARE]    = p_BLACK_ROOK;\
    st->board[BLACK_KING_STARTINGSQUARE]            = p_EMPTY_SQUARE;\
    st->board[BLACK_KING_QUEENSIDE_CASTLESQUARE]    = p_BLACK_KING;

enum smf_state_result black_castling_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;

    if (st->is_down) {
        if (st->pin_number == BLACK_KING_STARTINGSQUARE) {
            st->state_val = black;
            smf_set_state(ctx_ptr, STATE(black));
        }
        else if (st->pin_number == BLACK_KING_KINGSIDE_CASTLESQUARE && st->black_kingside) {
            st->state_val = black_castling_kingside_kingdown;
            smf_set_state(ctx_ptr, STATE(black_castling_kingside_kingdown));
        }
        else if (st->pin_number == BLACK_KING_QUEENSIDE_CASTLESQUARE && st->black_queenside ) {
            st->state_val = black_castling_queenside_kingdown;
            smf_set_state(ctx_ptr, STATE(black_castling_queenside_kingdown));
        }
        else {
            st->state_val = black_move;
            st->x = BLACK_KING_STARTINGSQUARE;
            smf_set_state(ctx_ptr, STATE(black_move));
        }
    }
    else if (st->is_black_piece) {
        if (st->board[st->pin_number] == p_BLACK_ROOK) {
            if (st->pin_number == BLACK_ROOK_KINGSIDE_STARTINGSQUARE && st->black_kingside) {
                st->state_val = black_castling_kingside_KINGUP_ROOKUP;
                smf_set_state(ctx_ptr, STATE(black_castling_kingside_KINGUP_ROOKUP));
            }
            else if (st->pin_number == BLACK_ROOK_QUEENSIDE_STARTINGSQUARE && st->black_queenside) {
                st->state_val = black_castling_queenside_KINGUP_ROOKUP;
                smf_set_state(ctx_ptr, STATE(black_castling_queenside_KINGUP_ROOKUP));
            }
            else {
                st->state_val = error;
                smf_set_state(ctx_ptr, STATE(error)); 
            }
        }
        else {
            st->state_val = error;
            smf_set_state(ctx_ptr, STATE(error));
        }
    }
    else if (st->is_white_piece) {
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
    struct chess_state* st  = (struct chess_state *)obj;

    if (st->is_up && st->board[st->pin_number] == p_BLACK_ROOK && st->pin_number == BLACK_ROOK_KINGSIDE_STARTINGSQUARE) {
        st->state_val = black_castling_kingside_kingdown_ROOKUP;
        smf_set_state(ctx_ptr, STATE(black_castling_kingside_kingdown_ROOKUP));
    }
    else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_castling_queenside_kingdown_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;

    if (st->is_up && st->board[st->pin_number] == p_BLACK_ROOK && st->pin_number == BLACK_ROOK_QUEENSIDE_STARTINGSQUARE) {
        st->state_val = black_castling_queenside_kingdown_ROOKUP;
        smf_set_state(ctx_ptr, STATE(black_castling_queenside_kingdown_ROOKUP));
    }
    else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_castling_kingside_kingdown_ROOKUP_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == BLACK_ROOK_KINGSIDE_CASTLESQUARE) {
        END_KINGSIDE_CASTLING_BLACK(st);
        st->state_val = white;
        smf_set_state(ctx_ptr, STATE(white));

        // send move
    }
    else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_castling_queenside_kingdown_ROOKUP_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == BLACK_ROOK_QUEENSIDE_CASTLESQUARE) {
        END_QUEENSIDE_CASTLING_BLACK(st);
        st->state_val = white;
        smf_set_state(ctx_ptr, STATE(white));

        // send move
    } 
    else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    } 

    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_castling_kingside_KINGUP_ROOKUP_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == BLACK_KING_KINGSIDE_CASTLESQUARE) {
        st->state_val = black_castling_kingside_kingdown_ROOKUP;
        smf_set_state(ctx_ptr, STATE(black_castling_kingside_kingdown_ROOKUP));
    }
    else if (st->is_down && st->pin_number == BLACK_ROOK_KINGSIDE_CASTLESQUARE) {
        st->state_val = black_castling_kingside_KINGUP_rookdown;
        smf_set_state(ctx_ptr, STATE(black_castling_kingside_KINGUP_rookdown));
    }
    else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_castling_queenside_KINGUP_ROOKUP_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == BLACK_KING_QUEENSIDE_CASTLESQUARE) {
        st->state_val = black_castling_queenside_kingdown_ROOKUP;
        smf_set_state(ctx_ptr, STATE(black_castling_queenside_kingdown_ROOKUP));
    }
    else if (st->is_down && st->pin_number == BLACK_ROOK_QUEENSIDE_CASTLESQUARE) {
        st->state_val = black_castling_queenside_KINGUP_rookdown;
        smf_set_state(ctx_ptr, STATE(black_castling_queenside_KINGUP_rookdown));
    }
    else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_castling_kingside_KINGUP_rookdown_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == BLACK_KING_KINGSIDE_CASTLESQUARE) {
        END_KINGSIDE_CASTLING_BLACK(st);
        st->state_val = white;
        smf_set_state(ctx_ptr, STATE(white));
    }
    else {
        st->state_val = error;
        smf_set_state(ctx_ptr, STATE(error));
    }

    return SMF_EVENT_HANDLED;
}

enum smf_state_result black_castling_queenside_KINGUP_rookdown_run(void *obj)
{
    struct smf_ctx *ctx_ptr = SMF_CTX(obj);
    struct chess_state* st  = (struct chess_state *)obj;

    if (st->is_down && st->pin_number == BLACK_KING_QUEENSIDE_CASTLESQUARE) {
        END_QUEENSIDE_CASTLING_BLACK(st);
        st->state_val = white;
        smf_set_state(ctx_ptr, STATE(white));
    }
    else {
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



// int main(void) 
// {
//     for (size_t i = 0; i < ARRAY_SIZE(starting_board_initializer); i++) {
//         state.board[i] = starting_board_initializer[i];
//     }

//     state.state_val = white;
//     state.black_kingside = true;
//     state.black_queenside = true;
//     state.white_kingside = true;
//     state.white_queenside = true;
//     state.en_passant = false;
//     state.ply_since_ponr = 0;
//     state.ply = 0;
//     state.x = 0;
//     state.y = 0;

//     while (1) {

//     }
// }

void reset_fsm(struct chess_state *st)
{
    /* probably get a mutex lock for this*/
    smf_set_terminate(SMF_CTX(st), -1);

    /* maybe wait for a semaphore or smth to see when the state has stopped executing */

    for (size_t i = 0; i < ARRAY_SIZE(starting_board_initializer); i++) {
        st->board[i] = starting_board_initializer[i];
    }


    st->state_val = white;
    st->black_kingside = true;
    st->black_queenside = true;
    st->white_kingside = true;
    st->white_queenside = true;
    st->en_passant = false;
    st->en_passant_square = 0;
    st->ply_since_ponr = 0;
    st->ply = 0;
    st->x = 0;
    st->y = 0;
    st->is_down = true;
    st->is_up = false;
    st->is_empty_square = false;
    st->is_white_piece = false;
    st->is_black_piece = false;
    st->is_started = false;
    st->piece = ' ';
    st->direct_to_error = false;
    //smf_set_initial(SMF_CTX(st), STATE(white));

}

void clean_state(struct chess_state *st) {
    reset_fsm(st);
    
    for (size_t i = 0; i < ARRAY_SIZE(empty_board_initializer); i++) {
        st->board[i] = empty_board_initializer[i];
    }
}

static void pin_change(struct chess_state *st, const uint8_t pin_number, const bool is_up) 
{
    st->pin_number = pin_number;
    st->is_up = is_up;
    st->is_down = !is_up;

    st->piece = st->board[st->pin_number];

    st->is_empty_square = (st->piece == ' ');
    st->is_black_piece = !(st->is_empty_square) && !(st->piece < 'a');
    st->is_white_piece = !(st->is_empty_square) && !(st->piece > 'a');

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

static void pin_change_test(struct chess_state *st, const uint8_t pin_number, const bool is_up, enum states starting_state) {
    st->pin_number = pin_number;
    st->is_up = is_up;
    st->is_down = !is_up;

    st->piece = st->board[st->pin_number];

    st->is_empty_square = (st->piece == ' ');
    st->is_black_piece = !(st->is_empty_square) && !(st->piece < 'a');
    st->is_white_piece = !(st->is_empty_square) && !(st->piece > 'a');

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

static void dump_state(const struct chess_state *st) 
{
    printk("Current state = %d\n", st->state_val);
    printk("BK = %s\n", st->black_kingside  ? "true" : "false");
    printk("BQ = %s\n", st->black_queenside ? "true" : "false");
    printk("WK = %s\n", st->white_kingside  ? "true" : "false");
    printk("WQ = %s\n", st->white_queenside ? "true" : "false");
    printk("EP = %s\n", st->en_passant      ? "true" : "false");
    printk("PSPONR = %d\n", st->ply_since_ponr);
    printk("PLY = %d\n", st->ply);
    printk("x = %d\n", st->x);
    printk("y = %d\n", st->y);
    printk( "up = %s\n", st->is_up ? "true" : "false");
    printk("down = %s\n", st->is_down ? "true" : "false");
    printk("black piece = %s\n", st->is_black_piece ? "true" : "false");
    printk("white piece = %s\n", st->is_white_piece ? "true" : "false");
}


ZTEST_SUITE(pin, NULL, NULL, NULL, NULL, NULL);

ZTEST(pin, pin_from_string) {
    int p = 0;
    zexpect_equal(pin("a8"), p);
}


ZTEST_SUITE(state, NULL, NULL, NULL, NULL, NULL);

ZTEST(state, state_reset) {
    reset_fsm(&state);
    zexpect_equal(state.state_val, white);
    zexpect_equal(state.black_kingside,  true);
    zexpect_equal(state.black_queenside, true);
    zexpect_equal(state.white_kingside,  true);
    zexpect_equal(state.white_queenside, true);
    zexpect_equal(state.en_passant, false);
    zexpect_equal(state.en_passant_square, 0);
    zexpect_equal(state.ply, 0);
    zexpect_equal(state.ply_since_ponr, 0);
    zexpect_equal(state.is_down, true);
    zexpect_equal(state.is_up, false);
    zexpect_equal(state.direct_to_error, false);
    for (size_t i = 0; i < ARRAY_SIZE(starting_board_initializer); i++) {
        zexpect_equal(state.board[i], starting_board_initializer[i]);
    }
}

ZTEST(state, clean_state) {
    clean_state(&state);
    zexpect_equal(state.state_val, white);
    zexpect_equal(state.black_kingside,  true);
    zexpect_equal(state.black_queenside, true);
    zexpect_equal(state.white_kingside,  true);
    zexpect_equal(state.white_queenside, true);
    zexpect_equal(state.en_passant, false);
    zexpect_equal(state.en_passant_square, 0);
    zexpect_equal(state.ply, 0);
    zexpect_equal(state.ply_since_ponr, 0);
    zexpect_equal(state.direct_to_error, false);
    for (size_t i = 0; i < ARRAY_SIZE(empty_board_initializer); i++) {
        zexpect_equal(state.board[i], empty_board_initializer[i]);
    }
}

ZTEST_SUITE(FSM_White, NULL, NULL, NULL, NULL, NULL);

ZTEST(FSM_White, white_move) {
    clean_state(&state);
    int w = 31;
    state.board[w] = p_WHITE_PAWN;
    pin_change(&state, w, true);
    zexpect_equal(state.state_val, white_move);
}

ZTEST(FSM_White, white_enemy_capture) {
    clean_state(&state);
    int b = 22;
    state.board[b] = p_BLACK_PAWN;
    pin_change(&state, b, true);
    zexpect_equal(state.state_val, white_enemy_capture);
}

ZTEST(FSM_White, undo_white_move) {
    reset_fsm(&state);
    pin_change(&state, (63-8), true);
    zexpect_equal(state.state_val, white_move);
    pin_change(&state, (63-8), false);
    zexpect_equal(state.state_val, white);
}

ZTEST(FSM_White, finish_white_move) {
    reset_fsm(&state);
    pin_change(&state, (63-8), true);
    zexpect_equal(state.state_val, white_move);
    pin_change(&state, (63-16), false);
    zexpect_equal(state.state_val, black);
}

ZTEST(FSM_White, start_white_capture) {
    reset_fsm(&state);
    int target = pin("a3");
    int from   = pin("b2");
    state.board[target] = p_BLACK_PAWN;
    pin_change(&state, from, true);
    pin_change(&state, target, true);
    zexpect_equal(state.state_val, white_capture);
}

ZTEST(FSM_White, finish_white_capture) {
    reset_fsm(&state);
    state.board[63-17] = 'p';
    pin_change(&state, (63-8), true);
    zexpect_equal(state.state_val, white_move);
    pin_change(&state, (63-17), true);
    zexpect_equal(state.state_val, white_capture);
    pin_change(&state, (63-17), false);
    zexpect_equal(state.state_val, black);
}

ZTEST_SUITE(FSM_Black, NULL, NULL, NULL, NULL, NULL);

ZTEST(FSM_Black, black_move) {
    clean_state(&state);
    state.state_val = black;
    // smf_set_state(SMF_CTX(&state), STATE(black));
    int from = pin("e5");
    state.board[from] = p_BLACK_PAWN;
    pin_change_test(&state, from, true, black);
    zexpect_equal(state.state_val, black_move);
}

ZTEST(FSM_Black, black_enemy_capture) {
    clean_state(&state);
    state.state_val = black;
    int from = pin("d4");
    state.board[from] = p_WHITE_PAWN;
    pin_change_test(&state, from, true, black);
    zexpect_equal(state.state_val, black_enemy_capture);
}

ZTEST(FSM_Black, undo_black_move) {
    clean_state(&state);
    state.state_val = black;
    int from = pin("e5");
    state.board[from] = p_BLACK_PAWN;
    pin_change_test(&state, from, true, black);
    zexpect_equal(state.state_val, black_move);
    pin_change(&state, from, false);
    zexpect_equal(state.state_val, black);

}

ZTEST(FSM_Black, finish_black_move) {
    reset_fsm(&state);
    state.state_val = black;
    int target = pin("e5");
    int from   = pin("e4");
    state.board[from] = p_BLACK_PAWN;
    pin_change_test(&state, from, true, black);
    zexpect_equal(state.state_val, black_move);
    pin_change(&state, target, false);
    zexpect_equal(state.state_val, white);
}

ZTEST(FSM_Black, black_capture) {
    reset_fsm(&state);
    state.state_val = black;
    int target = pin("a6");
    int from = pin("b7");
    state.board[target] = p_WHITE_PAWN;
    pin_change_test(&state, from, true, black);
    zexpect_equal(state.state_val, black_move);
    pin_change(&state, target, true);
    zexpect_equal(state.state_val, black_capture);
}

ZTEST(FSM_Black, start_black_enemy_capture) {
    reset_fsm(&state);
    state.state_val = black;
    int target = pin("a6");
    int from = pin("b7");
    state.board[target] = p_WHITE_PAWN;
    pin_change_test(&state, target, true, black);
    zexpect_equal(state.state_val, black_enemy_capture);
    pin_change(&state, from, true);
    zexpect_equal(state.state_val, black_capture);
}

ZTEST(FSM_Black, finish_black_capture) {
    reset_fsm(&state);
    state.state_val = black;
    int target = pin("a6");
    int from = pin("b7");
    state.board[target] = p_WHITE_PAWN;
    pin_change_test(&state, from, true, black);
    zexpect_equal(state.state_val, black_move);
    pin_change(&state, target, true);
    zexpect_equal(state.state_val, black_capture);
    pin_change(&state, target, false);
    zexpect_equal(state.state_val, white);
}

ZTEST(FSM_Black, finish_black_enemy_capture) {
    reset_fsm(&state);
    state.state_val = black;
    int target = pin("a6");
    int from = pin("b7");
    state.board[target] = p_WHITE_PAWN;
    pin_change_test(&state, target, true, black);
    zexpect_equal(state.state_val, black_enemy_capture);
    pin_change(&state, from, true);
    zexpect_equal(state.state_val, black_capture);
    pin_change(&state, target, false);
    zexpect_equal(state.state_val, white);
}

ZTEST_SUITE(FSM_Promotion, NULL, NULL, NULL, NULL, NULL);

ZTEST(FSM_Promotion, white_promotion) {
    clean_state(&state);
    int target = pin("a8");
    int from = pin("a7");
    state.board[from] = p_WHITE_PAWN;
    pin_change(&state, from, true);
    pin_change(&state, target, false);
    zexpect_equal(state.state_val, black);
    zexpect_equal(state.board[from], p_EMPTY_SQUARE);
    zexpect_equal(state.board[target], p_WHITE_QUEEN);

    for (size_t i = 0; i < 8; i++) {
        clean_state(&state);
        int target = pin("a8") + i;
        int from = pin("a7") + i;
        state.board[from] = p_WHITE_PAWN;
        pin_change(&state, from, true);
        pin_change(&state, target, false);
        zexpect_equal(state.state_val, black);
        zexpect_equal(state.board[from], p_EMPTY_SQUARE);
        zexpect_equal(state.board[target], p_WHITE_QUEEN);
    }
}

ZTEST(FSM_Promotion, black_promotion) {
    clean_state(&state);
    int target = pin("a1");
    int from = pin("a2");
    state.board[from] = p_BLACK_PAWN;
    pin_change_test(&state, from, true, black);
    pin_change(&state, target, false);
    zexpect_equal(state.state_val, white);
    zexpect_equal(state.board[from], p_EMPTY_SQUARE);
    zexpect_equal(state.board[target], p_BLACK_QUEEN);

    for (size_t i = 0; i < 8; i++) {
        clean_state(&state);
        int target = pin("a1") + i;
        int from = pin("a2") + i;
        state.board[from] = p_BLACK_PAWN;
        pin_change_test(&state, from, true, black);
        pin_change(&state, target, false);
        zexpect_equal(state.state_val, white);
        zexpect_equal(state.board[from], p_EMPTY_SQUARE);
        zexpect_equal(state.board[target], p_BLACK_QUEEN);
    }
}

ZTEST(FSM_Promotion, white_capture_promotion) {
    clean_state(&state);
    int target = pin("a8");
    int from = pin("b7");
    state.board[from] = p_WHITE_PAWN;
    state.board[target] = p_BLACK_PAWN;
    pin_change(&state, from, true);
    pin_change(&state, target, true);
    pin_change(&state, target, false);
    zexpect_equal(state.state_val, black);
    zexpect_equal(state.board[from], p_EMPTY_SQUARE);
    zexpect_equal(state.board[target], p_WHITE_QUEEN);
}

ZTEST(FSM_Promotion, white_enemy_capture_promotion) {
    clean_state(&state);
    int target = pin("a8");
    int from = pin("b7");
    state.board[from] = p_WHITE_PAWN;
    state.board[target] = p_BLACK_PAWN;
    pin_change(&state, target, true);
    zexpect_equal(state.state_val, white_enemy_capture);
    pin_change(&state, from, true);
    zexpect_equal(state.state_val, white_capture);
    pin_change(&state, target, false);
    zexpect_equal(state.state_val, black);
    zexpect_equal(state.board[from], p_EMPTY_SQUARE);
    zexpect_equal(state.board[target], p_WHITE_QUEEN);
}

ZTEST(FSM_Promotion, black_capture_promotion) {
    clean_state(&state);
    int target = pin("a1");
    int from = pin("b2");
    state.board[from] = p_BLACK_PAWN;
    state.board[target] = p_WHITE_PAWN;
    pin_change_test(&state, from, true, black);
    pin_change(&state, target, true);
    pin_change(&state, target, false);
    dump_state(&state);
    zexpect_equal(state.state_val, white);
    zexpect_equal(state.board[from], p_EMPTY_SQUARE);
    zexpect_equal(state.board[target], p_BLACK_QUEEN);
}

ZTEST(FSM_Promotion, black_enemy_capture_promotion) {
    clean_state(&state);
    int target = pin("a1");
    int from = pin("b2");
    state.board[from] = p_BLACK_PAWN;
    state.board[target] = p_WHITE_PAWN;
    pin_change_test(&state, target, true, black);
    pin_change(&state, from, true);
    pin_change(&state, target, false);
    zexpect_equal(state.state_val, white);
    zexpect_equal(state.board[from], p_EMPTY_SQUARE);
    zexpect_equal(state.board[target], p_BLACK_QUEEN);    
}

ZTEST(FSM_Promotion, white_not_promotion) {
    clean_state(&state);
    int target = pin("a8");
    int from = pin("b7");
    state.board[from] = p_WHITE_BISHOP;
    pin_change(&state, from, true);
    pin_change(&state, target, false);
    zexpect_equal(state.state_val, black);
    zexpect_equal(state.board[from], p_EMPTY_SQUARE);
    zexpect_equal(state.board[target], p_WHITE_BISHOP);
}

ZTEST_SUITE(FSM_en_passant, NULL, NULL, NULL, NULL, NULL);

ZTEST(FSM_en_passant, long_en_passant) {
    clean_state(&state);
    int from = pin("a2");
    int to = pin("a4");
    int en_passant_expected = pin("a3");
    state.board[from] = p_WHITE_PAWN;
    pin_change(&state, from, true);
    pin_change(&state, to, false);
    zassert_equal(state.state_val, black);
    zexpect_true(state.en_passant);
    zassert_equal(state.en_passant_square, en_passant_expected);
    
    from = pin("h7");
    to = pin("h5");
    en_passant_expected = pin("h6");
    state.board[from] = p_BLACK_PAWN;
    pin_change(&state, from, true);
    pin_change(&state, to, false);
    zexpect_true(state.en_passant);
    zassert_equal(state.en_passant_square, en_passant_expected);

    from = pin("a4");
    to = pin("a5");
    en_passant_expected = pin("a4");
    pin_change(&state, from, true);
    pin_change(&state, to, false);
    zexpect_false(state.en_passant);

	from = pin("h5");
	to = pin("h4");
	en_passant_expected = pin("h5");
	pin_change(&state, from, true);
	pin_change(&state, to, false);
	zexpect_false(state.en_passant);

    from = pin("g2");
    to = pin("g4");
    en_passant_expected = pin("g3");
    state.board[from] = p_WHITE_PAWN;
    pin_change(&state, from, true);
    pin_change(&state, to, false);
    zassert_equal(state.state_val, black);
    zassert_true(state.en_passant);
    zassert_equal(state.en_passant_square, en_passant_expected);

    from = pin("h4");
    to = pin("g3");
    int expected_empty = pin("g4");
    pin_change(&state, from, true);
    pin_change(&state, expected_empty, true);
    pin_change(&state, to, false);
    zassert_equal(state.state_val, white);
    zassert_equal(state.board[from], p_EMPTY_SQUARE);
    zassert_equal(state.board[to], p_BLACK_PAWN);
    zassert_equal(state.board[expected_empty], p_EMPTY_SQUARE);

    state.state_val = black;
    smf_set_state(SMF_CTX(&state), STATE(black));
    from = pin("b7");
    to = pin("b5");
    state.board[from] = p_BLACK_PAWN;
    en_passant_expected = pin("b6");
    pin_change(&state, from, true);
    pin_change(&state, to, false);
    zassert_true(state.en_passant);
    zassert_equal(state.en_passant_square, en_passant_expected);

    from = pin("a5");
    to = pin("b6");
    expected_empty = pin("b5");
    pin_change(&state, expected_empty, true);
    pin_change(&state, from, true);
    pin_change(&state, to, false);
    zassert_equal(state.state_val, black);
    zassert_equal(state.board[from], p_EMPTY_SQUARE);
    zassert_equal(state.board[to], p_WHITE_PAWN);
    zassert_equal(state.board[expected_empty], p_EMPTY_SQUARE); 
}

ZTEST(FSM_en_passant, test_all_en_passant_squares) {
    reset_fsm(&state);
    int to;
    int from;
    int en_passant_expected;
    zassert_equal(state.state_val, white);
    
    for (size_t i = 0; i < 8; i++) {
        from = pin("a2") + i;
        to = pin("a4") + i;
        en_passant_expected = pin("a3") + i;
        pin_change(&state, from, true);
        pin_change(&state, to, false);
        zassert_equal(state.state_val, black);
        zexpect_true(state.en_passant);
        zexpect_equal(state.en_passant_square, en_passant_expected);

        from = pin("a7") + i;
        to = pin("a5") + i;
        en_passant_expected = pin("a6") + i;
        pin_change(&state, from, true);
        pin_change(&state, to, false);
        zassert_equal(state.state_val, white);
        zexpect_true(state.en_passant);
        zexpect_equal(state.en_passant_square, en_passant_expected);
    }
    zassert_equal(state.ply, 16);
}

ZTEST_SUITE(FSM_castling, NULL, NULL, NULL, NULL, NULL);

static int count_pieces(struct chess_state *st) 
{
    int cnt = 0;
    for (size_t i = 0; i < ARRAY_SIZE(state.board); i++) {
        if (st->board[i] != p_EMPTY_SQUARE) {
            cnt++;
        }
    }
    return cnt;
} 

ZTEST(FSM_castling, black_kingside_castle_1__krkr) {
    clean_state(&state);
    state.state_val = black;
    state.board[BLACK_ROOK_KINGSIDE_STARTINGSQUARE] = p_BLACK_ROOK;
    state.board[BLACK_KING_STARTINGSQUARE] = p_BLACK_KING;
    zassert_true(state.black_kingside);
    zassert_equal(count_pieces(&state), 2);
    pin_change_test(&state, pin("e8"), true, black);
    zassert_equal(state.state_val, black_castling);
    pin_change(&state, pin("h8"), true);
    zassert_equal(state.state_val, black_castling_kingside_KINGUP_ROOKUP);
    pin_change(&state, pin("g8"), false);
    zassert_equal(state.state_val, black_castling_kingside_kingdown_ROOKUP);
    pin_change(&state, pin("f8"), false);
    zassert_equal(state.state_val, white);
    zassert_false(state.en_passant);
    zassert_false(state.black_kingside);
    zassert_false(state.black_queenside);
    zassert_equal(state.board[pin("g8")], p_BLACK_KING);
    zassert_equal(state.board[pin("f8")], p_BLACK_ROOK);
    zassert_equal(count_pieces(&state), 2);
}

ZTEST(FSM_castling, black_kingside_castle_2__krrk) {
    clean_state(&state);
    state.state_val = black;
    state.board[BLACK_ROOK_KINGSIDE_STARTINGSQUARE] = p_BLACK_ROOK;
    state.board[BLACK_KING_STARTINGSQUARE] = p_BLACK_KING;
    pin_change_test(&state, pin("e8"), true, black);
    zassert_equal(state.state_val, black_castling);
    pin_change(&state, pin("h8"), true);
    zassert_equal(state.state_val, black_castling_kingside_KINGUP_ROOKUP);
    pin_change(&state, pin("f8"), false);
    zassert_equal(state.state_val, black_castling_kingside_KINGUP_rookdown);
    pin_change(&state, pin("g8"), false);
    zassert_equal(state.state_val, white);
    zassert_false(state.en_passant);
    zassert_false(state.black_kingside);
    zassert_false(state.black_queenside);
    zassert_equal(state.board[pin("g8")], p_BLACK_KING);
    zassert_equal(state.board[pin("f8")], p_BLACK_ROOK);
    zassert_equal(count_pieces(&state), 2);
}

ZTEST(FSM_castling, black_kingside_castle_3__kkrr) {
    clean_state(&state);
    state.state_val = black;
    state.board[BLACK_ROOK_KINGSIDE_STARTINGSQUARE] = p_BLACK_ROOK;
    state.board[BLACK_KING_STARTINGSQUARE] = p_BLACK_KING;
    pin_change_test(&state, pin("e8"), true, black);
    zassert_equal(state.state_val, black_castling);
    pin_change(&state, pin("g8"), false);
    zassert_equal(state.state_val, black_castling_kingside_kingdown);
    pin_change(&state, pin("h8"), true);
    zassert_equal(state.state_val, black_castling_kingside_kingdown_ROOKUP);
    pin_change(&state, pin("f8"), false);
    zassert_equal(state.state_val, white);
    zassert_false(state.en_passant);
    zassert_false(state.black_kingside);
    zassert_false(state.black_queenside);
    zassert_equal(state.board[pin("g8")], p_BLACK_KING);
    zassert_equal(state.board[pin("f8")], p_BLACK_ROOK);
    zassert_equal(count_pieces(&state), 2);
}

ZTEST(FSM_castling, black_queenside_castle_1__krkr) {
    clean_state(&state);
    state.state_val = black;
    state.board[BLACK_ROOK_QUEENSIDE_STARTINGSQUARE] = p_BLACK_ROOK;
    state.board[BLACK_KING_STARTINGSQUARE] = p_BLACK_KING;
    zassert_true(state.black_queenside);
    zassert_equal(count_pieces(&state), 2);
    pin_change_test(&state, pin("e8"), true, black);
    zassert_equal(state.state_val, black_castling);
    pin_change(&state, pin("a8"), true);
    zassert_equal(state.state_val, black_castling_queenside_KINGUP_ROOKUP);
    pin_change(&state, pin("c8"), false);
    zassert_equal(state.state_val, black_castling_queenside_kingdown_ROOKUP);
    pin_change(&state, pin("d8"), false);
    zassert_equal(state.state_val, white);
    zassert_false(state.black_kingside);
    zassert_false(state.black_queenside);
    zassert_equal(state.board[pin("c8")], p_BLACK_KING);
    zassert_equal(state.board[pin("d8")], p_BLACK_ROOK);
    zassert_equal(count_pieces(&state), 2);
}

ZTEST(FSM_castling, black_queenside_castle_2__krrk) {
    clean_state(&state);
    state.state_val = black;
    state.board[BLACK_ROOK_QUEENSIDE_STARTINGSQUARE] = p_BLACK_ROOK;
    state.board[BLACK_KING_STARTINGSQUARE] = p_BLACK_KING;
    zassert_true(state.black_queenside);
    zassert_equal(count_pieces(&state), 2);

    pin_change_test(&state, pin("e8"), true, black);
    zassert_equal(state.state_val, black_castling);
    pin_change(&state, pin("a8"), true);
    zassert_equal(state.state_val, black_castling_queenside_KINGUP_ROOKUP);
    pin_change(&state, pin("d8"), false);
    zassert_equal(state.state_val, black_castling_queenside_KINGUP_rookdown);
    pin_change(&state, pin("c8"), false);

    zassert_equal(state.state_val, white);
    zassert_false(state.black_kingside);
    zassert_false(state.black_queenside);
    zassert_equal(state.board[pin("c8")], p_BLACK_KING);
    zassert_equal(state.board[pin("d8")], p_BLACK_ROOK);
    zassert_equal(count_pieces(&state), 2);
}

ZTEST(FSM_castling, black_queenside_castle_3__kkrr) {
    clean_state(&state);
    state.state_val = black;
    state.board[BLACK_ROOK_QUEENSIDE_STARTINGSQUARE] = p_BLACK_ROOK;
    state.board[BLACK_KING_STARTINGSQUARE] = p_BLACK_KING;
    zassert_true(state.black_queenside);
    zassert_equal(count_pieces(&state), 2);    

    pin_change_test(&state, pin("e8"), true, black);
    zassert_equal(state.state_val, black_castling);
    pin_change(&state, pin("c8"), false);
    zassert_equal(state.state_val, black_castling_queenside_kingdown);
    pin_change(&state, pin("a8"), true);
    zassert_equal(state.state_val, black_castling_queenside_kingdown_ROOKUP);
    pin_change(&state, pin("d8"), false);

    zassert_equal(state.state_val, white);
    zassert_false(state.black_kingside);
    zassert_false(state.black_queenside);
    zassert_equal(state.board[pin("c8")], p_BLACK_KING);
    zassert_equal(state.board[pin("d8")], p_BLACK_ROOK);
    zassert_equal(count_pieces(&state), 2);
}

ZTEST(FSM_castling, white_kingside_castle_1__krkr) {
    clean_state(&state);
    state.board[WHITE_ROOK_KINGSIDE_STARTINGSQUARE] = p_WHITE_ROOK;
    state.board[WHITE_KING_STARTINGSQUARE] = p_WHITE_KING;
    zassert_true(state.white_kingside);
    zassert_equal(count_pieces(&state), 2);
    pin_change(&state, pin("e1"), true);
    zassert_equal(state.state_val, white_castling);
    pin_change(&state, pin("h1"), true);
    zassert_equal(state.state_val, white_castling_kingside_KINGUP_ROOKUP);
    pin_change(&state, pin("g1"), false);
    zassert_equal(state.state_val, white_castling_kingside_kingdown_ROOKUP);
    pin_change(&state, pin("f1"), false);
    zassert_equal(state.state_val, black);
    zassert_false(state.en_passant);
    zassert_false(state.white_kingside);
    zassert_false(state.white_queenside);
    zassert_equal(state.board[pin("g1")], p_WHITE_KING);
    zassert_equal(state.board[pin("f1")], p_WHITE_ROOK);
    zassert_equal(count_pieces(&state), 2);
}

ZTEST(FSM_castling, white_kingside_castle_2__krrk) {
    clean_state(&state);
    state.board[WHITE_ROOK_KINGSIDE_STARTINGSQUARE] = p_WHITE_ROOK;
    state.board[WHITE_KING_STARTINGSQUARE] = p_WHITE_KING;
    zassert_true(state.white_kingside);
    zassert_equal(count_pieces(&state), 2);
    pin_change(&state, pin("e1"), true);
    zassert_equal(state.state_val, white_castling);
    pin_change(&state, pin("h1"), true);
    zassert_equal(state.state_val, white_castling_kingside_KINGUP_ROOKUP);
    pin_change(&state, pin("f1"), false);
    zassert_equal(state.state_val, white_castling_kingside_KINGUP_rookdown);
    pin_change(&state, pin("g1"), false);
    zassert_equal(state.state_val, black);
    zassert_false(state.en_passant);
    zassert_false(state.white_kingside);
    zassert_false(state.white_queenside);
    zassert_equal(state.board[pin("g1")], p_WHITE_KING);
    zassert_equal(state.board[pin("f1")], p_WHITE_ROOK);
    zassert_equal(count_pieces(&state), 2);

}

ZTEST(FSM_castling, white_kingside_castle_3__kkrr) {
    clean_state(&state);
    state.board[WHITE_ROOK_KINGSIDE_STARTINGSQUARE] = p_WHITE_ROOK;
    state.board[WHITE_KING_STARTINGSQUARE] = p_WHITE_KING; 
    zassert_true(state.white_kingside);
    zassert_equal(count_pieces(&state), 2);

    pin_change(&state, pin("e1"), true);
    zassert_equal(state.state_val, white_castling);
    pin_change(&state, pin("g1"), false);
    zassert_equal(state.state_val, white_castling_kingside_kingdown);
    pin_change(&state, pin("h1"), true);
    zassert_equal(state.state_val, white_castling_kingside_kingdown_ROOKUP);
    pin_change(&state, pin("f1"), false);
    zassert_equal(state.state_val, black);
    zassert_false(state.en_passant);
    zassert_false(state.white_kingside);
    zassert_false(state.white_queenside);
    zassert_equal(state.board[pin("g1")], p_WHITE_KING);
    zassert_equal(state.board[pin("f1")], p_WHITE_ROOK);
    zassert_equal(count_pieces(&state), 2);
}

ZTEST(FSM_castling, white_queenside_castle_1__krkr) {
    clean_state(&state);
    state.board[WHITE_ROOK_QUEENSIDE_STARTINGSQUARE] = p_WHITE_ROOK;
    state.board[WHITE_KING_STARTINGSQUARE] = p_WHITE_KING;
    zassert_true(state.white_queenside);
    zassert_equal(count_pieces(&state), 2);
    
    pin_change(&state, pin("e1"), true);
    zassert_equal(state.state_val, white_castling);
    pin_change(&state, pin("a1"), true);
    zassert_equal(state.state_val, white_castling_queenside_KINGUP_ROOKUP);
    pin_change(&state, pin("c1"), false);
    zassert_equal(state.state_val, white_castling_queenside_kingdown_ROOKUP);
    pin_change(&state, pin("d1"), false);

    zassert_equal(state.state_val, black);
    zassert_false(state.en_passant);
    zassert_false(state.white_kingside);
    zassert_false(state.white_queenside);
    zassert_equal(state.board[pin("c1")], p_WHITE_KING);
    zassert_equal(state.board[pin("d1")], p_WHITE_ROOK);
    zassert_equal(count_pieces(&state), 2);

}

ZTEST(FSM_castling, white_queenside_castle_2__krrk) {
    clean_state(&state);
    state.board[WHITE_ROOK_QUEENSIDE_STARTINGSQUARE] = p_WHITE_ROOK;
    state.board[WHITE_KING_STARTINGSQUARE] = p_WHITE_KING;
    zassert_true(state.white_queenside);
    zassert_equal(count_pieces(&state), 2);
    
    pin_change(&state, pin("e1"), true);
    zassert_equal(state.state_val, white_castling);
    pin_change(&state, pin("a1"), true);
    zassert_equal(state.state_val, white_castling_queenside_KINGUP_ROOKUP);
    pin_change(&state, pin("d1"), false);
    zassert_equal(state.state_val, white_castling_queenside_KINGUP_rookdown);
    pin_change(&state, pin("c1"), false);

    zassert_equal(state.state_val, black);
    zassert_false(state.en_passant);
    zassert_false(state.white_kingside);
    zassert_false(state.white_queenside);
    zassert_equal(state.board[pin("c1")], p_WHITE_KING);
    zassert_equal(state.board[pin("d1")], p_WHITE_ROOK);
    zassert_equal(count_pieces(&state), 2);
}

ZTEST(FSM_castling, white_queenside_castle_3__kkrr) {
    clean_state(&state);
    state.board[WHITE_ROOK_QUEENSIDE_STARTINGSQUARE] = p_WHITE_ROOK;
    state.board[WHITE_KING_STARTINGSQUARE] = p_WHITE_KING;
    zassert_true(state.white_queenside);
    zassert_equal(count_pieces(&state), 2);
    
    pin_change(&state, pin("e1"), true);
    zassert_equal(state.state_val, white_castling);
    pin_change(&state, pin("c1"), false);
    zassert_equal(state.state_val, white_castling_queenside_kingdown);
    pin_change(&state, pin("a1"), true);
    zassert_equal(state.state_val, white_castling_queenside_kingdown_ROOKUP);
    pin_change(&state, pin("d1"), false);

    zassert_equal(state.state_val, black);
    zassert_false(state.en_passant);
    zassert_false(state.white_kingside);
    zassert_false(state.white_queenside);
    zassert_equal(state.board[pin("c1")], p_WHITE_KING);
    zassert_equal(state.board[pin("d1")], p_WHITE_ROOK);
    zassert_equal(count_pieces(&state), 2);
}

#undef STATE
