#ifdef CONFIG_ZTEST

ZTEST_SUITE(pin, NULL, NULL, NULL, NULL, NULL);

ZTEST(pin, pin_from_string)
{
    int p = 0;
    zexpect_equal(pin("a8"), p);
}

ZTEST_SUITE(state, NULL, NULL, NULL, NULL, NULL);

ZTEST(state, state_reset)
{
    reset_fsm(&state);
    zexpect_equal(state.state_val, white);
    zexpect_equal(state.black_kingside, true);
    zexpect_equal(state.black_queenside, true);
    zexpect_equal(state.white_kingside, true);
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

ZTEST(state, clean_state)
{
    clean_state(&state);
    zexpect_equal(state.state_val, white);
    zexpect_equal(state.black_kingside, true);
    zexpect_equal(state.black_queenside, true);
    zexpect_equal(state.white_kingside, true);
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

ZTEST(FSM_White, white_move)
{
    clean_state(&state);
    int w          = 31;
    state.board[w] = p_WHITE_PAWN;
    pin_change(&state, w, true);
    zexpect_equal(state.state_val, white_move);
}

ZTEST(FSM_White, white_enemy_capture)
{
    clean_state(&state);
    int b          = 22;
    state.board[b] = p_BLACK_PAWN;
    pin_change(&state, b, true);
    zexpect_equal(state.state_val, white_enemy_capture);
}

ZTEST(FSM_White, undo_white_move)
{
    reset_fsm(&state);
    pin_change(&state, (63 - 8), true);
    zexpect_equal(state.state_val, white_move);
    pin_change(&state, (63 - 8), false);
    zexpect_equal(state.state_val, white);
}

ZTEST(FSM_White, finish_white_move)
{
    reset_fsm(&state);
    pin_change(&state, (63 - 8), true);
    zexpect_equal(state.state_val, white_move);
    pin_change(&state, (63 - 16), false);
    zexpect_equal(state.state_val, black);
}

ZTEST(FSM_White, start_white_capture)
{
    reset_fsm(&state);
    int target          = pin("a3");
    int from            = pin("b2");
    state.board[target] = p_BLACK_PAWN;
    pin_change(&state, from, true);
    pin_change(&state, target, true);
    zexpect_equal(state.state_val, white_capture);
}

ZTEST(FSM_White, finish_white_capture)
{
    reset_fsm(&state);
    state.board[63 - 17] = 'p';
    pin_change(&state, (63 - 8), true);
    zexpect_equal(state.state_val, white_move);
    pin_change(&state, (63 - 17), true);
    zexpect_equal(state.state_val, white_capture);
    pin_change(&state, (63 - 17), false);
    zexpect_equal(state.state_val, black);
}

ZTEST_SUITE(FSM_Black, NULL, NULL, NULL, NULL, NULL);

ZTEST(FSM_Black, black_move)
{
    clean_state(&state);
    state.state_val = black;
    // smf_set_state(SMF_CTX(&state), STATE(black));
    int from          = pin("e5");
    state.board[from] = p_BLACK_PAWN;
    pin_change_test(&state, from, true, black);
    zexpect_equal(state.state_val, black_move);
}

ZTEST(FSM_Black, black_enemy_capture)
{
    clean_state(&state);
    state.state_val   = black;
    int from          = pin("d4");
    state.board[from] = p_WHITE_PAWN;
    pin_change_test(&state, from, true, black);
    zexpect_equal(state.state_val, black_enemy_capture);
}

ZTEST(FSM_Black, undo_black_move)
{
    clean_state(&state);
    state.state_val   = black;
    int from          = pin("e5");
    state.board[from] = p_BLACK_PAWN;
    pin_change_test(&state, from, true, black);
    zexpect_equal(state.state_val, black_move);
    pin_change(&state, from, false);
    zexpect_equal(state.state_val, black);
}

ZTEST(FSM_Black, finish_black_move)
{
    reset_fsm(&state);
    state.state_val   = black;
    int target        = pin("e5");
    int from          = pin("e4");
    state.board[from] = p_BLACK_PAWN;
    pin_change_test(&state, from, true, black);
    zexpect_equal(state.state_val, black_move);
    pin_change(&state, target, false);
    zexpect_equal(state.state_val, white);
}

ZTEST(FSM_Black, black_capture)
{
    reset_fsm(&state);
    state.state_val     = black;
    int target          = pin("a6");
    int from            = pin("b7");
    state.board[target] = p_WHITE_PAWN;
    pin_change_test(&state, from, true, black);
    zexpect_equal(state.state_val, black_move);
    pin_change(&state, target, true);
    zexpect_equal(state.state_val, black_capture);
}

ZTEST(FSM_Black, start_black_enemy_capture)
{
    reset_fsm(&state);
    state.state_val     = black;
    int target          = pin("a6");
    int from            = pin("b7");
    state.board[target] = p_WHITE_PAWN;
    pin_change_test(&state, target, true, black);
    zexpect_equal(state.state_val, black_enemy_capture);
    pin_change(&state, from, true);
    zexpect_equal(state.state_val, black_capture);
}

ZTEST(FSM_Black, finish_black_capture)
{
    reset_fsm(&state);
    state.state_val     = black;
    int target          = pin("a6");
    int from            = pin("b7");
    state.board[target] = p_WHITE_PAWN;
    pin_change_test(&state, from, true, black);
    zexpect_equal(state.state_val, black_move);
    pin_change(&state, target, true);
    zexpect_equal(state.state_val, black_capture);
    pin_change(&state, target, false);
    zexpect_equal(state.state_val, white);
}

ZTEST(FSM_Black, finish_black_enemy_capture)
{
    reset_fsm(&state);
    state.state_val     = black;
    int target          = pin("a6");
    int from            = pin("b7");
    state.board[target] = p_WHITE_PAWN;
    pin_change_test(&state, target, true, black);
    zexpect_equal(state.state_val, black_enemy_capture);
    pin_change(&state, from, true);
    zexpect_equal(state.state_val, black_capture);
    pin_change(&state, target, false);
    zexpect_equal(state.state_val, white);
}

ZTEST_SUITE(FSM_Promotion, NULL, NULL, NULL, NULL, NULL);

ZTEST(FSM_Promotion, white_promotion)
{
    clean_state(&state);
    int target        = pin("a8");
    int from          = pin("a7");
    state.board[from] = p_WHITE_PAWN;
    pin_change(&state, from, true);
    pin_change(&state, target, false);
    zexpect_equal(state.state_val, black);
    zexpect_equal(state.board[from], p_EMPTY_SQUARE);
    zexpect_equal(state.board[target], p_WHITE_QUEEN);

    for (size_t i = 0; i < 8; i++) {
        clean_state(&state);
        int target        = pin("a8") + i;
        int from          = pin("a7") + i;
        state.board[from] = p_WHITE_PAWN;
        pin_change(&state, from, true);
        pin_change(&state, target, false);
        zexpect_equal(state.state_val, black);
        zexpect_equal(state.board[from], p_EMPTY_SQUARE);
        zexpect_equal(state.board[target], p_WHITE_QUEEN);
    }
}

ZTEST(FSM_Promotion, black_promotion)
{
    clean_state(&state);
    int target        = pin("a1");
    int from          = pin("a2");
    state.board[from] = p_BLACK_PAWN;
    pin_change_test(&state, from, true, black);
    pin_change(&state, target, false);
    zexpect_equal(state.state_val, white);
    zexpect_equal(state.board[from], p_EMPTY_SQUARE);
    zexpect_equal(state.board[target], p_BLACK_QUEEN);

    for (size_t i = 0; i < 8; i++) {
        clean_state(&state);
        int target        = pin("a1") + i;
        int from          = pin("a2") + i;
        state.board[from] = p_BLACK_PAWN;
        pin_change_test(&state, from, true, black);
        pin_change(&state, target, false);
        zexpect_equal(state.state_val, white);
        zexpect_equal(state.board[from], p_EMPTY_SQUARE);
        zexpect_equal(state.board[target], p_BLACK_QUEEN);
    }
}

ZTEST(FSM_Promotion, white_capture_promotion)
{
    clean_state(&state);
    int target          = pin("a8");
    int from            = pin("b7");
    state.board[from]   = p_WHITE_PAWN;
    state.board[target] = p_BLACK_PAWN;
    pin_change(&state, from, true);
    pin_change(&state, target, true);
    pin_change(&state, target, false);
    zexpect_equal(state.state_val, black);
    zexpect_equal(state.board[from], p_EMPTY_SQUARE);
    zexpect_equal(state.board[target], p_WHITE_QUEEN);
}

ZTEST(FSM_Promotion, white_enemy_capture_promotion)
{
    clean_state(&state);
    int target          = pin("a8");
    int from            = pin("b7");
    state.board[from]   = p_WHITE_PAWN;
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

ZTEST(FSM_Promotion, black_capture_promotion)
{
    clean_state(&state);
    int target          = pin("a1");
    int from            = pin("b2");
    state.board[from]   = p_BLACK_PAWN;
    state.board[target] = p_WHITE_PAWN;
    pin_change_test(&state, from, true, black);
    pin_change(&state, target, true);
    pin_change(&state, target, false);
    zexpect_equal(state.state_val, white);
    zexpect_equal(state.board[from], p_EMPTY_SQUARE);
    zexpect_equal(state.board[target], p_BLACK_QUEEN);
}

ZTEST(FSM_Promotion, black_enemy_capture_promotion)
{
    clean_state(&state);
    int target          = pin("a1");
    int from            = pin("b2");
    state.board[from]   = p_BLACK_PAWN;
    state.board[target] = p_WHITE_PAWN;
    pin_change_test(&state, target, true, black);
    pin_change(&state, from, true);
    pin_change(&state, target, false);
    zexpect_equal(state.state_val, white);
    zexpect_equal(state.board[from], p_EMPTY_SQUARE);
    zexpect_equal(state.board[target], p_BLACK_QUEEN);
}

ZTEST(FSM_Promotion, white_not_promotion)
{
    clean_state(&state);
    int target        = pin("a8");
    int from          = pin("b7");
    state.board[from] = p_WHITE_BISHOP;
    pin_change(&state, from, true);
    pin_change(&state, target, false);
    zexpect_equal(state.state_val, black);
    zexpect_equal(state.board[from], p_EMPTY_SQUARE);
    zexpect_equal(state.board[target], p_WHITE_BISHOP);
}

ZTEST_SUITE(FSM_en_passant, NULL, NULL, NULL, NULL, NULL);

ZTEST(FSM_en_passant, long_en_passant)
{
    clean_state(&state);
    int from                = pin("a2");
    int to                  = pin("a4");
    int en_passant_expected = pin("a3");
    state.board[from]       = p_WHITE_PAWN;
    pin_change(&state, from, true);
    pin_change(&state, to, false);
    zassert_equal(state.state_val, black);
    zexpect_true(state.en_passant);
    zassert_equal(state.en_passant_square, en_passant_expected);

    from                = pin("h7");
    to                  = pin("h5");
    en_passant_expected = pin("h6");
    state.board[from]   = p_BLACK_PAWN;
    pin_change(&state, from, true);
    pin_change(&state, to, false);
    zexpect_true(state.en_passant);
    zassert_equal(state.en_passant_square, en_passant_expected);

    from                = pin("a4");
    to                  = pin("a5");
    en_passant_expected = pin("a4");
    pin_change(&state, from, true);
    pin_change(&state, to, false);
    zexpect_false(state.en_passant);

    from                = pin("h5");
    to                  = pin("h4");
    en_passant_expected = pin("h5");
    pin_change(&state, from, true);
    pin_change(&state, to, false);
    zexpect_false(state.en_passant);

    from                = pin("g2");
    to                  = pin("g4");
    en_passant_expected = pin("g3");
    state.board[from]   = p_WHITE_PAWN;
    pin_change(&state, from, true);
    pin_change(&state, to, false);
    zassert_equal(state.state_val, black);
    zassert_true(state.en_passant);
    zassert_equal(state.en_passant_square, en_passant_expected);

    from               = pin("h4");
    to                 = pin("g3");
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
    from                = pin("b7");
    to                  = pin("b5");
    state.board[from]   = p_BLACK_PAWN;
    en_passant_expected = pin("b6");
    pin_change(&state, from, true);
    pin_change(&state, to, false);
    zassert_true(state.en_passant);
    zassert_equal(state.en_passant_square, en_passant_expected);

    from           = pin("a5");
    to             = pin("b6");
    expected_empty = pin("b5");
    pin_change(&state, expected_empty, true);
    pin_change(&state, from, true);
    pin_change(&state, to, false);
    zassert_equal(state.state_val, black);
    zassert_equal(state.board[from], p_EMPTY_SQUARE);
    zassert_equal(state.board[to], p_WHITE_PAWN);
    zassert_equal(state.board[expected_empty], p_EMPTY_SQUARE);
}

ZTEST(FSM_en_passant, test_all_en_passant_squares)
{
    reset_fsm(&state);
    int to;
    int from;
    int en_passant_expected;
    zassert_equal(state.state_val, white);

    for (size_t i = 0; i < 8; i++) {
        from                = pin("a2") + i;
        to                  = pin("a4") + i;
        en_passant_expected = pin("a3") + i;
        pin_change(&state, from, true);
        pin_change(&state, to, false);
        zassert_equal(state.state_val, black);
        zexpect_true(state.en_passant);
        zexpect_equal(state.en_passant_square, en_passant_expected);

        from                = pin("a7") + i;
        to                  = pin("a5") + i;
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

ZTEST(FSM_castling, black_kingside_castle_1__krkr)
{
    clean_state(&state);
    state.state_val                                 = black;
    state.board[BLACK_ROOK_KINGSIDE_STARTINGSQUARE] = p_BLACK_ROOK;
    state.board[BLACK_KING_STARTINGSQUARE]          = p_BLACK_KING;
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

ZTEST(FSM_castling, black_kingside_castle_2__krrk)
{
    clean_state(&state);
    state.state_val                                 = black;
    state.board[BLACK_ROOK_KINGSIDE_STARTINGSQUARE] = p_BLACK_ROOK;
    state.board[BLACK_KING_STARTINGSQUARE]          = p_BLACK_KING;
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

ZTEST(FSM_castling, black_kingside_castle_3__kkrr)
{
    clean_state(&state);
    state.state_val                                 = black;
    state.board[BLACK_ROOK_KINGSIDE_STARTINGSQUARE] = p_BLACK_ROOK;
    state.board[BLACK_KING_STARTINGSQUARE]          = p_BLACK_KING;
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

ZTEST(FSM_castling, black_queenside_castle_1__krkr)
{
    clean_state(&state);
    state.state_val                                  = black;
    state.board[BLACK_ROOK_QUEENSIDE_STARTINGSQUARE] = p_BLACK_ROOK;
    state.board[BLACK_KING_STARTINGSQUARE]           = p_BLACK_KING;
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

ZTEST(FSM_castling, black_queenside_castle_2__krrk)
{
    clean_state(&state);
    state.state_val                                  = black;
    state.board[BLACK_ROOK_QUEENSIDE_STARTINGSQUARE] = p_BLACK_ROOK;
    state.board[BLACK_KING_STARTINGSQUARE]           = p_BLACK_KING;
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

ZTEST(FSM_castling, black_queenside_castle_3__kkrr)
{
    clean_state(&state);
    state.state_val                                  = black;
    state.board[BLACK_ROOK_QUEENSIDE_STARTINGSQUARE] = p_BLACK_ROOK;
    state.board[BLACK_KING_STARTINGSQUARE]           = p_BLACK_KING;
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

ZTEST(FSM_castling, white_kingside_castle_1__krkr)
{
    clean_state(&state);
    state.board[WHITE_ROOK_KINGSIDE_STARTINGSQUARE] = p_WHITE_ROOK;
    state.board[WHITE_KING_STARTINGSQUARE]          = p_WHITE_KING;
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

ZTEST(FSM_castling, white_kingside_castle_2__krrk)
{
    clean_state(&state);
    state.board[WHITE_ROOK_KINGSIDE_STARTINGSQUARE] = p_WHITE_ROOK;
    state.board[WHITE_KING_STARTINGSQUARE]          = p_WHITE_KING;
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

ZTEST(FSM_castling, white_kingside_castle_3__kkrr)
{
    clean_state(&state);
    state.board[WHITE_ROOK_KINGSIDE_STARTINGSQUARE] = p_WHITE_ROOK;
    state.board[WHITE_KING_STARTINGSQUARE]          = p_WHITE_KING;
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

ZTEST(FSM_castling, white_queenside_castle_1__krkr)
{
    clean_state(&state);
    state.board[WHITE_ROOK_QUEENSIDE_STARTINGSQUARE] = p_WHITE_ROOK;
    state.board[WHITE_KING_STARTINGSQUARE]           = p_WHITE_KING;
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

ZTEST(FSM_castling, white_queenside_castle_2__krrk)
{
    clean_state(&state);
    state.board[WHITE_ROOK_QUEENSIDE_STARTINGSQUARE] = p_WHITE_ROOK;
    state.board[WHITE_KING_STARTINGSQUARE]           = p_WHITE_KING;
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

ZTEST(FSM_castling, white_queenside_castle_3__kkrr)
{
    clean_state(&state);
    state.board[WHITE_ROOK_QUEENSIDE_STARTINGSQUARE] = p_WHITE_ROOK;
    state.board[WHITE_KING_STARTINGSQUARE]           = p_WHITE_KING;
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

ZTEST(FSM_castling, cancel_into_move)
{
    clean_state(&state);
    state.board[WHITE_ROOK_QUEENSIDE_STARTINGSQUARE] = p_WHITE_ROOK;
    state.board[WHITE_KING_STARTINGSQUARE]           = p_WHITE_KING;
    pin_change(&state, pin("e1"), true);
    zassert_equal(state.state_val, white_castling);
    pin_change(&state, pin("e2"), false);
    zassert_equal(state.state_val, black);

    clean_state(&state);
    state.state_val                                  = black;
    state.board[BLACK_ROOK_QUEENSIDE_STARTINGSQUARE] = p_BLACK_ROOK;
    state.board[BLACK_KING_STARTINGSQUARE]           = p_BLACK_KING;
    pin_change_test(&state, pin("e8"), true, black);
    zassert_equal(state.state_val, black_castling);
    pin_change(&state, pin("e7"), false);
    zassert_equal(state.state_val, white);
}

ZTEST(FSM_castling, cancel_into_capture)
{
    clean_state(&state);
    state.board[pin("e2")]                 = p_BLACK_KNIGHT;
    state.board[WHITE_KING_STARTINGSQUARE] = p_WHITE_KING;
    pin_change(&state, pin("e1"), true);
    zassert_equal(state.state_val, white_castling);
    pin_change(&state, pin("e2"), true);
    zassert_equal(state.state_val, white_capture);
    pin_change(&state, pin("e2"), false);
    zassert_equal(state.state_val, black);
    zassert_equal(state.board[pin("e1")], p_EMPTY_SQUARE);
    zassert_equal(state.board[pin("e2")], p_WHITE_KING);
    zassert_equal(count_pieces(&state), 1);

    clean_state(&state);
    state.state_val                        = black;
    state.board[pin("e7")]                 = p_WHITE_KNIGHT;
    state.board[BLACK_KING_STARTINGSQUARE] = p_BLACK_KING;
    pin_change_test(&state, pin("e8"), true, black);
    zassert_equal(state.state_val, black_castling);
    pin_change(&state, pin("e7"), true);
    zassert_equal(state.state_val, black_capture);
    pin_change(&state, pin("e7"), false);
    zassert_equal(state.state_val, white);
    zassert_equal(state.board[pin("e8")], p_EMPTY_SQUARE);
    zassert_equal(state.board[pin("e7")], p_BLACK_KING);
    zassert_equal(count_pieces(&state), 1);
}

ZTEST(FSM_castling, cancel_into_undo)
{
    clean_state(&state);
    state.board[WHITE_KING_STARTINGSQUARE] = p_WHITE_KING;
    pin_change(&state, pin("e1"), true);
    zassert_equal(state.state_val, white_castling);
    pin_change(&state, pin("e1"), false);
    zassert_equal(state.state_val, white);

    clean_state(&state);
    state.state_val                        = black;
    state.board[BLACK_KING_STARTINGSQUARE] = p_BLACK_KING;
    pin_change_test(&state, pin("e8"), true, black);
    zassert_equal(state.state_val, black_castling);
    pin_change(&state, pin("e8"), false);
    zassert_equal(state.state_val, black);
}

ZTEST_SUITE(FSM_async, NULL, NULL, NULL, NULL, NULL);

ZTEST(FSM_async, wq_white_move)
{
    scope_defer(cleanup_linked_fsm_nodes)(&move_check_list);
    reset_fsm(&state);

    queue_fsm_work_fifo(&state, (63 - 8), true);
    queue_fsm_work_fifo(&state, (63 - 16), false);
    k_usleep(500);
    zexpect_equal(state.state_val, black);

    struct fsm_state_node *ptr;

    k_sem_take(&sem_list_accessible, K_FOREVER);

    ptr = SYS_SLIST_PEEK_HEAD_CONTAINER(&move_check_list, ptr, node);
    zexpect_equal(ptr->state, white_move);
    ptr = SYS_SLIST_PEEK_NEXT_CONTAINER(ptr, node);
    zexpect_equal(ptr->state, black);
}

ZTEST(FSM_async, wq_finish_white_capture)
{
    scope_defer(cleanup_linked_fsm_nodes)(&move_check_list);
    reset_fsm(&state);
    state.board[63 - 17] = 'p';

    queue_fsm_work_fifo(&state, (63 - 8), true);
    queue_fsm_work_fifo(&state, (63 - 17), true);
    queue_fsm_work_fifo(&state, (63 - 17), false);
    k_usleep(500);
    zexpect_equal(state.state_val, black);

    struct fsm_state_node *ptr;
    LOG_DBG("FIRST");
    k_sem_take(&sem_list_accessible, K_FOREVER);
    ptr = SYS_SLIST_PEEK_HEAD_CONTAINER(&move_check_list, ptr, node);
    zexpect_equal(ptr->state, white_move);
    LOG_DBG("SECOND");
    k_sem_take(&sem_list_accessible, K_FOREVER);
    ptr = SYS_SLIST_PEEK_NEXT_CONTAINER(ptr, node);
    zexpect_equal(ptr->state, white_capture);
    LOG_DBG("THIRD");
    k_sem_take(&sem_list_accessible, K_FOREVER);
    ptr = SYS_SLIST_PEEK_NEXT_CONTAINER(ptr, node);
    zexpect_equal(ptr->state, black);
}

ZTEST(FSM_async, white_queenside_castle_1__krkr_wq)
{

    scope_defer(cleanup_linked_fsm_nodes)(&move_check_list);
    clean_state(&state);
    state.board[WHITE_ROOK_QUEENSIDE_STARTINGSQUARE] = p_WHITE_ROOK;
    state.board[WHITE_KING_STARTINGSQUARE]           = p_WHITE_KING;
    zassert_true(state.white_queenside);
    zassert_equal(count_pieces(&state), 2);

    queue_fsm_work_fifo(&state, pin("e1"), true);
    queue_fsm_work_fifo(&state, pin("a1"), true);
    queue_fsm_work_fifo(&state, pin("c1"), false);
    queue_fsm_work_fifo(&state, pin("d1"), false);

    struct fsm_state_node *ptr;

    k_sem_take(&sem_list_accessible, K_FOREVER);
    sys_snode_t *n_ptr = sys_slist_peek_head(&move_check_list);

    ptr = SYS_SLIST_CONTAINER(n_ptr, ptr, node);
    zexpect_equal(ptr->state, white_castling);
    k_sem_take(&sem_list_accessible, K_FOREVER);
    ptr = SYS_SLIST_PEEK_NEXT_CONTAINER(ptr, node);
    zexpect_equal(ptr->state, white_castling_queenside_KINGUP_ROOKUP);
    k_sem_take(&sem_list_accessible, K_FOREVER);
    ptr = SYS_SLIST_PEEK_NEXT_CONTAINER(ptr, node);
    zexpect_equal(ptr->state, white_castling_queenside_kingdown_ROOKUP);
    k_sem_take(&sem_list_accessible, K_FOREVER);
    ptr = SYS_SLIST_PEEK_NEXT_CONTAINER(ptr, node);
    zexpect_equal(ptr->state, black);
}

ZTEST_SUITE(PB, NULL, NULL, NULL, NULL, NULL);

bool encode_msg(uint8_t *buffer, size_t buf_size, size_t *msg_len)
{
    bool ret;
    BoardMove mv = BoardMove_init_zero;

    mv.is_up = true;
    mv.pin   = 13;

    pb_ostream_t stream = pb_ostream_from_buffer(buffer, buf_size);

    ret      = pb_encode(&stream, BoardMove_fields, &mv);
    *msg_len = stream.bytes_written;

    if (!ret) {
        PRINT("Encoding error: %s\n", PB_GET_ERROR(&stream));
    }
    return ret;
}

bool decode_msg(uint8_t *buffer, size_t msg_len, BoardMove *out)
{
    bool ret;
    *out = (BoardMove)BoardMove_init_zero;

    pb_istream_t stream = pb_istream_from_buffer(buffer, msg_len);

    ret = pb_decode(&stream, BoardMove_fields, out);

    PRINT("pin: %d\nis_up: %s\n", out->pin, out->is_up ? "true" : "false");

    if (!ret) {
        PRINT("Decoding error: %s\n", PB_GET_ERROR(&stream));
    } else {
        pb_release(BoardMove_fields, out);
    }

    return ret;
}

ZTEST(PB, encode)
{
    uint8_t buf[BoardMove_size] = { 0 };

    size_t written = 0;

    bool success = encode_msg(buf, BoardMove_size, &written);

    zassert_equal(success, true);
}

ZTEST(PB, decode)
{
    uint8_t buf[BoardMove_size];
    BoardMove mv = BoardMove_init_zero;

    size_t written = 0;

    bool success = encode_msg(buf, BoardMove_size, &written);
    zassert_equal(success, true);
    success = decode_msg(buf, BoardMove_size, &mv);

    zassert_equal(mv.is_up, true);
    zassert_equal(mv.pin, 13);
}

#endif // #ifdef CONFIG_ZTEST
#undef STATE
