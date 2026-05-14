#ifndef SMAK_INCLUDED_SMAK_UTIL_H_
#define SMAK_INCLUDED_SMAK_UTIL_H_

typedef struct smak_chess_move smak_chess_move_t;
struct smak_json_move_obj_internal;
struct smak_json_game_obj_internal;

extern const char* smak_gamestate_strings[];
extern const char* move_type_strings[];

void test_smak_json_print(void);

enum move_type {
    EN_PASSANT,
    CASTLING,
    PROMOTION,
    NORMAL,
    MOVE_TYPE_MAX
};

enum smak_gamestate {
    WHITE_WIN = 0,
    BLACK_WIN = 1,
    DRAW_AGREED = 2,
    DRAW_REPETITION = 3,
    DRAW_FIFTY_MOVES = 4,
    WHITE_TO_MOVE = 5,
    BLACK_TO_MOVE = 6,
    NOT_STARTED = 7,
    ERROR = 8
};

#endif // SMAK_INCLUDED_SMAK_UTIL_H_