#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include "board.h"
#include "movegen.h"
#include "uci.h"

uint64_t perft(board_t* board, const bool print, const int depth) {
  if (depth == 0) {
    return 1;
  }

  uint64_t nodes = 0;

  move_list_t move_list = gen_color_moves(board);
  for (uint8_t i = 0; i < move_list.len; i++) {
    const move_t move = move_list.moves[i];
    const undo_t undo = do_move(move, board);
    if (!was_legal(move, board)) {
      undo_move(undo, move, board);
      continue;
    }

    const uint64_t subnodes = perft(board, false, depth - 1);
    nodes += subnodes;

    if (print) {
      char move_uci[6] = {0};
      move_to_uci(move, move_uci);
      UCI_SEND("%s: %" PRIu64, move_uci, subnodes);
    }

    undo_move(undo, move, board);
  }

  return nodes;
}
