#ifndef SMAK_INCLUDED_FSM_H_
#define SMAK_INCLUDED_FSM_H_

#include "model.h"
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

/* clang-format off */

struct chess_state;

void reset_fsm(struct chess_state *st);

struct fsm_state_node {
    sys_snode_t node;
    enum states state;
};

void queue_fsm_work_fifo(struct chess_state *st, int pin, bool is_up);

const char *state_board_get(struct chess_state *st);

struct chess_state *state_get(void);

sys_slist_t *move_check_list_get(void);

/* clang-format on */

#endif // SMAK_INCLUDED_FSM_H_