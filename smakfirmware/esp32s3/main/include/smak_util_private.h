#ifndef SMAK_INCLUDED_SMAK_UTIL_PRIVATE_H_
#define SMAK_INCLUDED_SMAK_UTIL_PRIVATE_H_

#include <smak.pb.h>
#include <smak_util.h>
#include <stdint.h>

struct smak_json_move_obj {
    uint64_t id;
    uint32_t ply;
    uint16_t from;
    uint16_t to;
    uint8_t piece[2];
    uint8_t captured[2];
    const uint8_t* move_type;
};

struct smak_json_game_obj {
    struct {
        uint64_t id;
    } board;
    enum smak_gamestate gamestate;
};

[[nodiscard("Return value MUST be checked to avoid NULL derefs")]]
int smak_json_move_obj_internal_get_from_pb(smak_chess_move_t* in, size_t mv_sz, struct smak_json_move_obj* out);

[[nodiscard("Return value MUST be checked to avoid NULL derefs")]]
char* smak_json_game_obj_to_str(const struct smak_json_game_obj* in);

[[nodiscard("Return value MUST be checked to avoid NULL derefs")]]
char* smak_json_move_obj_to_str(struct smak_json_move_obj* obj);

#endif // SMAK_INCLUDED_SMAK_UTIL_PRIVATE_H_
