#include "fsm.h"
#include "model.h"
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(smak_main_shell, LOG_LEVEL_DBG);

static struct chess_state *state;
static sys_slist_t *move_check_list;
extern struct smak_json_obj shell_obj;

static void print_board(const struct shell *sh, const char *board)
{
    shell_fprintf_normal(sh, "     A   B   C   D   E   F   G   H \n");
    for (size_t i = 0; i < 8; i++) {
        shell_fprintf_normal(sh, "   +---+---+---+---+---+---+---+---+\n %d ",
                             8 - i);
        for (size_t j = 0; j < 8; j++) {
            shell_fprintf_normal(sh, "| %c ", board[i * 8 + j]);
        }
        shell_fprintf_normal(sh, "|\n");
    }

    shell_fprintf_normal(sh, "   +---+---+---+---+---+---+---+---+\n");
}

static void state_handler(const struct shell *sh, size_t argc, char **argv,
                          void *data)
{

    struct fsm_state_node *ptr = SYS_SLIST_PEEK_TAIL_CONTAINER(move_check_list, ptr, node);

    if (ptr) {
        shell_fprintf_normal(sh, "Current state is %s\n",
                             state_strings[ptr->state]);
    } else {
        shell_fprintf_normal(sh, "Current state is not accessible\n");
    }
}

void print_board_handler(const struct shell *sh)
{
    print_board(sh, state_board_get(state));
}

void test_json_cmd(const struct shell *sh)
{

    char buf[512] = { 0 };

    encode_smak_obj(&shell_obj, buf, 512);

    shell_fprintf_normal(sh, "%s\n", buf);
}

static void piece_handler(const struct shell *sh, size_t argc, char **argv,
                          void *data)
{

    if ((strcmp(argv[0], "reset")) == 0) {
        reset_fsm(state);
        shell_fprintf_normal(sh, "State machine was reset\n");
    }

    if (argc != 2) {
        return;
    }

    if (strlen(argv[1]) != 2) {
        return;
    }

    if ((strcmp(argv[0], "lift")) == 0) {
        queue_fsm_work_fifo(state, pin(argv[1]), true);
    } else if ((strcmp(argv[0], "put")) == 0) {
        queue_fsm_work_fifo(state, pin(argv[1]), false);
        k_usleep(100);
        print_board(sh, state_board_get(state));
    }
}

static void move_handler(const struct shell *sh, size_t argc, char **argv,
                         void *data)
{
    if (argc < 2) {
        LOG_ERR("You must make a move!");
        return;
    }

    if (strlen(argv[1]) != 4) {
        LOG_WRN("Wrong move type!");
        return;
    }

    const char *from = argv[1];
    const char *to   = argv[1] + 2;

    shell_fprintf_normal(sh, "You made a move from pin %d to pin %d\n", pin(from),
                         pin(to));
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_smak,
    SHELL_CMD_ARG(move, NULL,
                  "Make a move in long algebraic notation, ie. e2e4",
                  move_handler, 1, 1),
    SHELL_CMD_ARG(state, NULL, "Get the state from the state machine",
                  state_handler, 0, 0),
    SHELL_CMD_ARG(lift, NULL,
                  "Lift a piece up from a square in algebraic notation",
                  piece_handler, 1, 1),
    SHELL_CMD_ARG(put, NULL,
                  "Put a piece down on a square in algebraic notation",
                  piece_handler, 1, 1),
    SHELL_CMD_ARG(reset, NULL, "Reset the state machine", piece_handler, 0, 0),
    SHELL_CMD_ARG(board, NULL, "View the board", print_board_handler, 0, 0),
    SHELL_CMD_ARG(jsontest, NULL, "JSON test", test_json_cmd, 0, 0),
    SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(smak, &sub_smak, "smak", NULL);

void test_handler(struct shell *sh)
{

    if (IS_ENABLED(CONFIG_ZTEST)) {
        shell_fprintf_normal(sh, "Testing is enabled\n");
    } else {
        shell_fprintf_normal(sh, "Testing is disabled\n");
    }
}

SHELL_CMD_REGISTER(is_test, NULL, "Check if we are in testing env",
                   test_handler);

#ifndef CONFIG_ZTEST

int main(void)
{
    state           = state_get();
    move_check_list = move_check_list_get();
}
#else
[[gnu::constructor]] void startup_shell(void)
{
    state           = state_get();
    move_check_list = move_check_list_get();
}
#endif
