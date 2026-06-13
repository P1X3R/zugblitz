#include "uci.h"

#include <inttypes.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "defs.h"
#include "history.h"
#include "misc.h"
#include "movegen.h"
#include "perft.h"
#include "search.h"
#include "transposition.h"

#define LINE_BUF_LEN 8192
#define FEN_BUF_LEN 128

uint64_t move_overhead_ms = 100;

static void stop_worker(void) { search_flag_store(ST_EXIT); }

static bool parse_fen_tokens(char* fen, char** saveptr) {
  int fields = 0;
  fen[0] = '\0';
  char* token;

  while (fields < 6 && (token = strtok_r(NULL, " ", saveptr)) != NULL) {
    if (strcmp(token, "moves") == 0) {
      break;
    }

    // Safety check
    if (strlen(fen) + strlen(token) + 2 >= FEN_BUF_LEN) {
      break;
    }

    if (fields > 0) {
      strcat(fen, " ");
    }
    strcat(fen, token);
    fields++;
  }
  return fields >= 4;
}

static bool apply_move_list(board_t* board, char** saveptr) {
  char* token;
  while ((token = strtok_r(NULL, " ", saveptr)) != NULL) {
    uint8_t i = 0;
    const move_list_t move_list = gen_color_moves(board);

    for (; i < move_list.len; i++) {
      const move_t move = move_list.moves[i];
      char move_uci[6] = {0};
      move_to_uci(move, move_uci);

      if (strcmp(token, move_uci) == 0) {
        do_move(move, board);
        break;
      }
    }

    if (i == move_list.len) {
      UCI_SEND("info string invalid move %s", token);
      return false;
    }
  }
  return true;
}

static void handle_position(engine_t* engine, char** saveptr) {
  char* token = strtok_r(NULL, " ", saveptr);
  if (!token) {
    goto bad_argument;
  }

  if (strcmp(token, "startpos") == 0) {
    engine->board =
        from_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
  } else if (strcmp(token, "fen") == 0) {
    char fen[FEN_BUF_LEN];
    if (!parse_fen_tokens(fen, saveptr)) {
      UCI_SEND("info string not enough fen parts");
      return;
    }
    engine->board = from_fen(fen);
  } else {
    goto bad_argument;
  }

  token = strtok_r(NULL, " ", saveptr);
  if (token && strcmp(token, "moves") == 0) {
    apply_move_list(&engine->board, saveptr);
  }
  return;

bad_argument:
  UCI_SEND("info string unknown position argument");
}

static void handle_go(engine_t* engine, pthread_t* worker,
                      uci_go_params_t* params, char** saveptr) {
  const uint64_t start_ms = now_ms();
  uint64_t wtime = 0, btime = 0, winc = 0, binc = 0, mtg = 0;
  uint64_t movetime = 0;
  uint8_t depth = MAX_PLY - 1;
  int perft_depth = 0;

  search_flag_store(ST_THINK);
  char* token;
  while ((token = strtok_r(NULL, " ", saveptr)) != NULL) {
    if (strcmp(token, "wtime") == 0) {
      const char* val = strtok_r(NULL, " ", saveptr);
      if (val) {
        wtime = (uint64_t)atoll(val);
      }
    } else if (strcmp(token, "btime") == 0) {
      const char* val = strtok_r(NULL, " ", saveptr);
      if (val) {
        btime = (uint64_t)atoll(val);
      }
    } else if (strcmp(token, "winc") == 0) {
      const char* val = strtok_r(NULL, " ", saveptr);
      if (val) {
        winc = (uint64_t)atoll(val);
      }
    } else if (strcmp(token, "binc") == 0) {
      const char* val = strtok_r(NULL, " ", saveptr);
      if (val) {
        binc = (uint64_t)atoll(val);
      }
    } else if (strcmp(token, "movestogo") == 0) {
      const char* val = strtok_r(NULL, " ", saveptr);
      if (val) {
        mtg = (uint64_t)atoll(val);
      }
    } else if (strcmp(token, "depth") == 0) {
      const char* val = strtok_r(NULL, " ", saveptr);
      if (val) {
        depth = (uint8_t)atoi(val);
        depth = (depth >= MAX_PLY) ? (MAX_PLY - 1) : depth;
      }
    } else if (strcmp(token, "movetime") == 0) {
      const char* val = strtok_r(NULL, " ", saveptr);
      if (val) {
        movetime = (uint64_t)atoll(val);
      }
    } else if (strcmp(token, "ponder") == 0) {
      search_flag_store(ST_PONDER);
    } else if (strcmp(token, "mate") == 0) {
      const char* val = strtok_r(NULL, " ", saveptr);
      if (val) {
        depth = (uint8_t)atoi(val) * 2 - 1;
        depth = (depth >= MAX_PLY) ? (MAX_PLY - 1) : depth;
      }
    } else if (strcmp(token, "perft") == 0) {
      const char* val = strtok_r(NULL, " ", saveptr);
      if (val) {
        perft_depth = atoi(val);
        perft_depth = (perft_depth >= MAX_PLY) ? (MAX_PLY - 1) : perft_depth;
      }
    }
  }

  if (perft_depth) {
    const uint64_t start_ms = now_ms();
    const uint64_t nodes = perft(&engine->board, true, perft_depth);
    const uint64_t duration_ms = now_ms() - start_ms;

    UCI_SEND("\nnodes: %" PRIu64, nodes);
    UCI_SEND("time: %" PRIu64 " ms", duration_ms);
    UCI_SEND("nps: %.4f N/s", (double)(nodes * 1000) / (double)duration_ms);
    return;
  }

  const uint64_t color_time_ms =
      (engine->board.side_to_move == CLR_WHITE) ? wtime : btime;
  const uint64_t color_inc_ms =
      (engine->board.side_to_move == CLR_WHITE) ? winc : binc;

  // Infinite search by default
  // Max time is `UINT64_MAX` on infinite search
  // Technically not infinite but it would search for 584,942,417 years.
  time_control_t time_control = {false, start_ms, UINT64_MAX, UINT64_MAX};

  if (color_time_ms > 0) {
    const uint64_t base_time = color_time_ms - move_overhead_ms;
    mtg = (mtg > 0) ? mtg : 20;
    const uint64_t allocated = (base_time / mtg) + (color_inc_ms / 2);

    time_control.soft_ms = allocated * 8 / 10;
    time_control.hard_ms = allocated * 12 / 10;
    time_control.hard_ms =
        (time_control.hard_ms > base_time) ? base_time : time_control.hard_ms;
  } else if (color_inc_ms > 0) {
    time_control.soft_ms = color_inc_ms * 8 / 10;
    time_control.hard_ms = color_inc_ms * 9 / 10;
  } else if (movetime > 0) {
    const uint64_t base_time =
        (movetime <= move_overhead_ms) ? 1 : movetime - move_overhead_ms;
    time_control.soft_ms = base_time * 9 / 10;
    time_control.hard_ms = base_time;
  }

  *params = (uci_go_params_t){engine, time_control, depth};
  if (pthread_create(worker, NULL, start_search, (void*)params) != 0) {
    UCI_SEND("info string error starting search thread");
  }

  // Detached: lifetime controlled exclusively by SEARCH_FLAG
  if (pthread_detach(*worker) != 0) {
    UCI_SEND("info string error detaching search thread");
  }
}

static void handle_option(char** saveptr) {
  char* token = strtok_r(NULL, " ", saveptr);  // should be "name"
  if (!token || strcmp(token, "name") != 0) {
    goto bad_argument;
  }

  // collect option name until "value"
  char option_name[128] = {0};
  while ((token = strtok_r(NULL, " ", saveptr)) != NULL &&
         strcmp(token, "value") != 0) {
    if (option_name[0]) {
      strcat(option_name, " ");
    }
    strcat(option_name, token);
  }

  if (!token) {
    goto bad_argument;  // no "value" found
  }

  token = strtok_r(NULL, " ", saveptr);  // the actual value
  if (!token) {
    goto bad_argument;
  }

  const int val = atoi(token);

  if (strcmp(option_name, "Hash") == 0) {
    if (val < 2) {
      UCI_SEND("info string Hash has to be at least 2 mb, using default 32 mb");
      tt_init(DEFAULT_TT_SIZE);
    } else if (val > 1024) {
      UCI_SEND("info string Hash capped at 1024 mb");
      tt_init(1024);
    } else {
      tt_init(val);
    }
    return;
  } else if (strcmp(option_name, "MoveOverhead") == 0) {
    if (val < 0) {
      UCI_SEND(
          "info string MoveOverhead cannot be negative, using default 100");
      move_overhead_ms = 100;
    } else if (val > 10000) {
      UCI_SEND("info string MoveOverhead capped at 10000 ms");
      move_overhead_ms = 10000;
    } else {
      move_overhead_ms = val;
    }
    return;
  } else if (strcmp(option_name, "Ponder") == 0) {
    // Pondering is always enabled
    return;
  }

bad_argument:
  UCI_SEND("info string unknown option argument");
}

void uci_loop(engine_t* engine) {
  char line[LINE_BUF_LEN] = {0};
  pthread_t worker;
  uci_go_params_t uci_go_struct = {engine, {0, 0, 0, 0}, 0};
  char* saveptr = NULL;

  while (fgets(line, sizeof line, stdin)) {
    line[strcspn(line, "\r\n")] = 0;
    char* token = strtok_r(line, " ", &saveptr);
    if (!token) {
      continue;
    }

    if (strcmp(token, "uci") == 0) {
      UCI_SEND("id name Zugblitz 1.3.2");
      UCI_SEND("id author P1x3r");
      UCI_SEND("option name Ponder type check default false");
      UCI_SEND("option name Hash type spin default 32 min 2 max 1024");
      UCI_SEND(
          "option name MoveOverhead type spin default 100 min 0 max 10000");
      UCI_SEND("uciok");
    } else if (strcmp(token, "isready") == 0) {
      stop_worker();
      UCI_SEND("readyok");
    } else if (strcmp(token, "position") == 0) {
      stop_worker();
      handle_position(engine, &saveptr);
    } else if (strcmp(token, "go") == 0) {
      stop_worker();
      handle_go(engine, &worker, &uci_go_struct, &saveptr);
    } else if (strcmp(token, "quit") == 0) {
      stop_worker();
      break;
    } else if (strcmp(token, "stop") == 0) {
      stop_worker();
    } else if (strcmp(token, "ponderhit") == 0) {
      if (search_flag_load() == ST_PONDER) {
        search_flag_store(ST_PONDERHIT);
      }
    } else if (strcmp(token, "ucinewgame") == 0) {
      stop_worker();
      hh_clear();
      tt_clear();
    } else if (strcmp(token, "setoption") == 0) {
      handle_option(&saveptr);
    } else if (strcmp(token, "board") == 0) {
      print_board(&engine->board);
    } else {
      UCI_SEND("info string unknown command");
    }
  }
  stop_worker();
}

void send_info_depth(search_ctx_t* ctx, const uint8_t depth, const int score) {
  const int score_sign = (score > 0) - (score < 0);
  const int encoded_score = (abs(score) > MATE_THRESHOLD)
                                ? (MATE_SCORE - abs(score) + 1) / 2 * score_sign
                                : score;
  const uint64_t search_duration_ms = now_ms() - ctx->time_control.start_ms + 1;
  const uint64_t nps = (ctx->nodes * 1000) / search_duration_ms;

  printf("info depth %d seldepth %d score %s %d time %" PRIu64 " nodes %" PRIu64
         " nps %" PRIu64 " hashfull %d pv ",
         depth, ctx->seldepth, (abs(score) > MATE_THRESHOLD) ? "mate" : "cp",
         encoded_score, search_duration_ms, ctx->nodes, nps, get_hashfull());

  for (uint8_t i = 0; i < ctx->pv.len[0]; i++) {
    const move_t move = ctx->pv.table[0][i];
    char move_uci[6] = {0};
    move_to_uci(move, move_uci);

    printf("%s ", move_uci);
  }

  putchar('\n');
  fflush(stdout);
}

void send_info_currmove(const move_t move, const uint8_t currmovenumber) {
  char move_uci[6] = {0};
  move_to_uci(move, move_uci);

  UCI_SEND("info currmove %s currmovenumber %d", move_uci, currmovenumber);
}
