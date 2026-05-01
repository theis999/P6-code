#ifndef SMAK_INCLUDED_MODEL_H_
#define SMAK_INCLUDED_MODEL_H_

#include <zephyr/kernel.h>

enum move_type { EN_PASSANT,
    CASTLING,
    PROMOTION,
    NORMAL,
    MOVE_TYPE_MAX };

struct smak_json_obj {
    uint64_t id;
    uint32_t ply;
    uint16_t from;
    uint16_t to;
    uint8_t piece;
    uint8_t captured;

    enum move_type movetype;
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

uint8_t pin(const char* restrict str);
#define SMAK_PIECES
#ifndef SMAK_PIECES
#define SMAK_PIECES
const char p_EMPTY_SQUARE = ' ';
const char p_BLACK_PAWN = 'p', p_WHITE_PAWN = 'P';
const char p_BLACK_ROOK = 'r', p_WHITE_ROOK = 'R';
[[maybe_unused]] const char p_BLACK_KNIGHT = 'n', p_WHITE_KNIGHT = 'N';
[[maybe_unused]] const char p_BLACK_BISHOP = 'b', p_WHITE_BISHOP = 'B';
const char p_BLACK_QUEEN = 'q', p_WHITE_QUEEN = 'Q';
const char p_BLACK_KING = 'k', p_WHITE_KING = 'K';

const int WHITE_KING_STARTINGSQUARE = 63 - 3;
const int BLACK_KING_STARTINGSQUARE = 0 + 4;

const int WHITE_ROOK_KINGSIDE_STARTINGSQUARE = 63 - 0;
const int WHITE_ROOK_QUEENSIDE_STARTINGSQUARE = 63 - 7;
const int BLACK_ROOK_KINGSIDE_STARTINGSQUARE = 0 + 7;
const int BLACK_ROOK_QUEENSIDE_STARTINGSQUARE = 0;

const int WHITE_KING_KINGSIDE_CASTLESQUARE = 63 - 1;
const int WHITE_KING_QUEENSIDE_CASTLESQUARE = 63 - 5;
const int BLACK_KING_KINGSIDE_CASTLESQUARE = 0 + 6;
const int BLACK_KING_QUEENSIDE_CASTLESQUARE = 0 + 2;

const int WHITE_ROOK_KINGSIDE_CASTLESQUARE = 63 - 2;
const int WHITE_ROOK_QUEENSIDE_CASTLESQUARE = 63 - 4;
const int BLACK_ROOK_KINGSIDE_CASTLESQUARE = 0 + 5;
const int BLACK_ROOK_QUEENSIDE_CASTLESQUARE = 0 + 3;

#endif

/* clang-format off */
enum smak_pieces {
    p_EMPTY_SQUARE = ' ',
    p_BLACK_PAWN = 'p', p_WHITE_PAWN = 'P',
    p_BLACK_ROOK = 'r', p_WHITE_ROOK = 'R',
    p_BLACK_KNIGHT = 'n', p_WHITE_KNIGHT = 'N',
    p_BLACK_BISHOP = 'b', p_WHITE_BISHOP = 'B',
    p_BLACK_QUEEN = 'q', p_WHITE_QUEEN = 'Q',
    p_BLACK_KING = 'k', p_WHITE_KING = 'K',

    WHITE_KING_STARTINGSQUARE = 63 - 3,
    BLACK_KING_STARTINGSQUARE = 0 + 4,

    WHITE_ROOK_KINGSIDE_STARTINGSQUARE = 63 - 0,
    WHITE_ROOK_QUEENSIDE_STARTINGSQUARE = 63 - 7,
    BLACK_ROOK_KINGSIDE_STARTINGSQUARE = 0 + 7,
    BLACK_ROOK_QUEENSIDE_STARTINGSQUARE = 0,

    WHITE_KING_KINGSIDE_CASTLESQUARE = 63 - 1,
    WHITE_KING_QUEENSIDE_CASTLESQUARE = 63 - 5,
    BLACK_KING_KINGSIDE_CASTLESQUARE = 0 + 6,
    BLACK_KING_QUEENSIDE_CASTLESQUARE = 0 + 2,

    WHITE_ROOK_KINGSIDE_CASTLESQUARE = 63 - 2,
    WHITE_ROOK_QUEENSIDE_CASTLESQUARE = 63 - 4,
    BLACK_ROOK_KINGSIDE_CASTLESQUARE = 0 + 5,
    BLACK_ROOK_QUEENSIDE_CASTLESQUARE = 0 + 3
};


extern const char *state_strings[error + 1];

extern const char board[64];

extern const char starting_board_initializer[64];

extern const char empty_board_initializer[64];

void encode_smak_obj(struct smak_json_obj* obj, char* buf, size_t buf_size);

/* clang-format on */
#endif