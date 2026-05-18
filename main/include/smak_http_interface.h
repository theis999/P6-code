#ifndef SMAK_INCLUDED_SMAK_HTTP_INTERFACE_H_
#define SMAK_INCLUDED_SMAK_HTTP_INTERFACE_H_

#include <smak.pb.h>
#include <smak_util.h>
#include <stdint.h>

struct smak_pb_ctx {
    char buffer[SMAK_CHESS_MOVE_SIZE];
    size_t msg_size;
};

struct smak_http_command {
    enum {
        SMAK_HTTP_MOVE_POST,
        SMAK_HTTP_GAME_POST,
        SMAK_HTTP_GAME_PATCH,
        SMAK_HTTP_AUTH_TOKEN_GET,
        SMAK_HTTP_USERS_GET,
    } cmd_type;
    union {
        smak_chess_move_t mv;
        struct smak_pb_ctx ctx;
    } data;
    uint64_t game_id;
};

typedef struct smak_http_command smak_http_command_t;
typedef struct smak_pb_ctx smak_pb_ctx_t;

extern char* smak_http_auth_token_get(const char* host);
extern void smak_http_post_move(uint64_t id, smak_chess_move_t* move, [[gnu::unused]] size_t pb_size);

#endif // SMAK_INCLUDED_SMAK_HTTP_INTERFACE_H_
