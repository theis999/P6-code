#include "fsm.h"
#include "model.h"
#include "src/smak.pb.h"
#include <stddef.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>

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

static const struct device *uart_usb = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0));

static bool uart_is_enabled = false;
struct uart_ctx {
    uint8_t *buf;
    size_t sz;
};

K_MSGQ_DEFINE(json_queue, sizeof(struct uart_ctx), 16, 1);

void uart_cb(const struct device *dev, void *user_data)
{
    ARG_UNUSED(user_data);
    while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {

        struct uart_ctx ctx = { 0 };

        if (uart_irq_tx_ready(dev)) {
            int ret = k_msgq_get(&json_queue, &ctx, K_NO_WAIT);
            if (ret < 0) {
                continue;
            }
            size_t sent = uart_fifo_fill(dev, ctx.buf, ctx.sz);
            if (sent < ctx.sz) {
                LOG_ERR("Data longer than space in uart FIFO");
            } else {
                break;
            }
        }
    }

    uart_irq_tx_disable(dev);
}

struct uart_pb_ctx {
    uint8_t *buf;
    size_t sz;
};
K_MSGQ_DEFINE(pb_queue, sizeof(struct uart_pb_ctx), 16, 1);

void uart_cb_pb(const struct device *dev, void *user_data)
{
    ARG_UNUSED(user_data);
    while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
        LOG_INF("%s loop entry", __func__);
        struct uart_pb_ctx ctx = { 0 };

        if (uart_irq_tx_ready(dev)) {
            int ret = k_msgq_get(&pb_queue, &ctx, K_NO_WAIT);
            if (ret < 0) {
                continue;
            }
            size_t sent = uart_fifo_fill(dev, ctx.buf, ctx.sz);
            if (sent < ctx.sz) {
                LOG_ERR("Data longer than space in uart FIFO");
            } else {
                break;
            }
        }
    }
    uart_irq_tx_disable(dev);
}

#ifdef SMAK_JSON_VERSION
void test_json_cmd(const struct shell *sh)
{

    LOG_INF("JSON TEST CMD ENTRY");
    char buf[512] = { 0 };
    if (!uart_is_enabled) {
        // uart_irq_callback_user_data_set(uart_usb, uart_cb, (void *)buf);
        uart_irq_callback_user_data_set(uart_usb, uart_cb, (void *)buf);
        uart_is_enabled = true;
    }

    encode_smak_obj(&shell_obj, buf, 512);

    size_t len = strlen(buf);
    buf[511]   = '\0';
    shell_fprintf_normal(sh, "%s\n", buf);
    if (len < 508) {
        buf[len++] = '\r';
        buf[len++] = '\n';
    }

    struct uart_ctx ctx = { .buf = buf, .sz = len };
    k_msgq_put(&json_queue, &ctx, K_NO_WAIT);

    // size_t enc_len = smak_move_to_pb(&shell_obj, buf);

    // struct uart_pb_ctx ctx = { .buf = buf, .sz = enc_len };

    LOG_DBG("Putting in queue");
    k_msgq_put(&json_queue, &ctx, K_FOREVER);

    uart_irq_tx_enable(uart_usb);

    while (!uart_irq_tx_complete(uart_usb)) { }
    // for (size_t i = 0; i < len; i++) {
    //     uart_poll_out(uart_usb, buf[i]);
    // }
    // uart_poll_out(uart_usb, '\r');
    // uart_poll_out(uart_usb, '\n');
}
#else
void test_json_cmd(const struct shell *sh)
{
    shell_fprintf_normal(sh, "%s entry\n", __func__);
    char buf[Smak_ChessMove_size] = { 0 };

    if (!uart_is_enabled) {
        int ret = uart_irq_callback_user_data_set(uart_usb, uart_cb, (void *)buf);
        if (ret < 0) {
            shell_fprintf_error(sh, "Failed to set uart irq\n");
        }
        uart_is_enabled = true;
    }
    size_t enc_len = smak_move_to_pb(&shell_obj, buf);
    shell_fprintf_normal(sh, "Encoded length is %u bytes\n", enc_len);
    struct uart_pb_ctx ctx = { .buf = buf, .sz = enc_len };
    // shell_fprintf_normal(sh, "Putting in queue\n");
    // // k_msgq_put(&pb_queue, &ctx, K_FOREVER);
    // shell_fprintf_normal(sh, "Put object in queue\n");

    for (size_t i = 0; i < enc_len; i++) {
        uart_poll_out(uart_usb, buf[i]);
    }
    // uart_irq_tx_enable(uart_usb);
    // while (!uart_irq_tx_complete(uart_usb)) { }
}
#endif

static void piece_handler(const struct shell *sh, size_t argc, char **argv, void *data)
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
