#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "bitboard.h"
#include "defs.h"
#include "misc.h"

#define MAX_VARIANTS 4096

typedef struct {
  int8_t rank, file;
} offset_t;
typedef struct {
  offset_t* offsets;
  uint8_t len;
} deltas_t;

static const offset_t KNIGHT_OFFSETS[] = {
    {+2, +1}, {+2, -1}, {-2, +1}, {-2, -1},
    {+1, +2}, {+1, -2}, {-1, +2}, {-1, -2},
};
static const deltas_t KNIGHT_DELTAS = {
    .offsets = (offset_t*)KNIGHT_OFFSETS,
    .len = 8,
};

static const offset_t KING_OFFSETS[] = {
    {+1, 0}, {-1, 0}, {0, +1}, {0, -1}, {+1, +1}, {+1, -1}, {-1, +1}, {-1, -1},
};
static const deltas_t KING_DELTAS = {
    .offsets = (offset_t*)KING_OFFSETS,
    .len = 8,
};

static const offset_t WPAWN_CAPTURE_OFFSETS[] = {
    {+1, +1},
    {+1, -1},
};

static const deltas_t WPAWN_CAPTURE_DELTAS = {
    .offsets = (offset_t*)WPAWN_CAPTURE_OFFSETS,
    .len = 2,
};

static const offset_t BPAWN_CAPTURE_OFFSETS[] = {
    {-1, +1},
    {-1, -1},
};
static const deltas_t BPAWN_CAPTURE_DELTAS = {
    .offsets = (offset_t*)BPAWN_CAPTURE_OFFSETS,
    .len = 2,
};

static const offset_t ROOK_OFFSETS[] = {
    {+1, 0},  // north
    {-1, 0},  // south
    {0, +1},  // east
    {0, -1},  // west
};
static const deltas_t ROOK_DELTAS = {
    .offsets = (offset_t*)ROOK_OFFSETS,
    .len = 4,
};

static const offset_t BISHOP_OFFSETS[] = {
    {+1, +1},  // northeast
    {+1, -1},  // northwest
    {-1, +1},  // southeast
    {-1, -1},  // southwest
};
static const deltas_t BISHOP_DELTAS = {
    .offsets = (offset_t*)BISHOP_OFFSETS,
    .len = 4,
};

bitboard_t gen_jumping_attacks(const square_t sq, const deltas_t deltas) {
  bitboard_t result = 0ULL;

  const int rank = get_rank(sq), file = get_file(sq);

  for (uint8_t i = 0; i < deltas.len; i++) {
    const int attack_rank = rank + deltas.offsets[i].rank,
              attack_file = file + deltas.offsets[i].file;
    if (is_valid_coord(attack_rank, attack_file)) {
      result |= bit(to_square(attack_rank, attack_file));
    }
  }

  return result;
}

bitboard_t gen_sliding_attacks(const square_t sq, const bitboard_t occupancy,
                               const deltas_t deltas) {
  bitboard_t result = 0ULL;

  const int rank = get_rank(sq), file = get_file(sq);

  for (uint8_t i = 0; i < deltas.len; i++) {
    int attack_rank = rank + deltas.offsets[i].rank,
        attack_file = file + deltas.offsets[i].file;

    while (is_valid_coord(attack_rank, attack_file)) {
      const bitboard_t attacked_bit = bit(to_square(attack_rank, attack_file));
      result |= attacked_bit;

      if (attacked_bit & occupancy) {
        break;
      }

      attack_rank += deltas.offsets[i].rank;
      attack_file += deltas.offsets[i].file;
    }
  }

  return result;
}

bitboard_t gen_edge_mask(const square_t sq) {
  bitboard_t result = 0;
  const bitboard_t current = bit(sq);

  const bitboard_t edges[4] = {R1, R8, FA, FH};
  for (uint8_t i = 0; i < 4; i++) {
    if (!(edges[i] & current)) {
      result |= edges[i];
    }
  }

  return result;
}

bitboard_t gen_occupancy(uint16_t variant, bitboard_t mask) {
  bitboard_t occupancy = 0;

  for (; variant; mask &= mask - 1, variant >>= 1) {
    if (variant & 1) {
      occupancy |= mask & -mask;
    }
  }

  return occupancy;
}

FORCE_INLINE uint64_t fewbits(void) {
  return random_u64() & random_u64() & random_u64();
}

uint64_t find_magics(const square_t sq, const bitboard_t mask, bitboard_t* used,
                     const deltas_t deltas) {
  const uint8_t bits = __builtin_popcountll(mask);
  const uint16_t len = 1U << bits;

  bitboard_t occupancies[MAX_VARIANTS] = {0}, attacks[MAX_VARIANTS] = {0};
  for (uint16_t variant = 0; variant < len; variant++) {
    occupancies[variant] = gen_occupancy(variant, mask);
    attacks[variant] = gen_sliding_attacks(sq, occupancies[variant], deltas);
  }

  for (unsigned try = 0; try < 10000000; try++) {
    const uint64_t magic = fewbits();
    bool collided = false;

    // Check for collisions
    for (uint16_t variant = 0; variant < len; variant++) {
      const uint16_t magic_index =
          (occupancies[variant] * magic) >> (64 - bits);

      if (used[magic_index] == 0ULL) {
        used[magic_index] = attacks[variant];
      } else if (used[magic_index] != attacks[variant]) {
        collided = true;
        break;
      }
    }

    if (!collided) {
      return magic;
    } else {
      memset(used, 0, len * sizeof(bitboard_t));
    }
  }

  printf("*** Failed ***\n");
  return 0;
}

int main(void) {
  srand(time(NULL));

  const size_t total_sliding = 512 * NR_OF_SQUARES + 4096 * NR_OF_SQUARES;
  size_t offset = 0;
  bitboard_t* sliding_attacks = calloc(total_sliding, sizeof(bitboard_t));

  bitboard_t white_pawn_attacks[NR_OF_SQUARES];
  bitboard_t black_pawn_attacks[NR_OF_SQUARES];
  bitboard_t knight_attacks[NR_OF_SQUARES];
  bitboard_t king_attacks[NR_OF_SQUARES];

  magic_t bishop_magics[NR_OF_SQUARES], rook_magics[NR_OF_SQUARES];

  for (square_t sq = SQ_A1; sq <= SQ_H8; sq++) {
    const bitboard_t edge_mask = ~gen_edge_mask(sq);
    const bitboard_t bishop_mask =
        gen_sliding_attacks(sq, 0ULL, BISHOP_DELTAS) & edge_mask;
    const bitboard_t rook_mask =
        gen_sliding_attacks(sq, 0ULL, ROOK_DELTAS) & edge_mask;

    const uint8_t bishop_bits = __builtin_popcountll(bishop_mask);
    const uint8_t rook_bits = __builtin_popcountll(rook_mask);

    bishop_magics[sq] = (magic_t){
        .magic = find_magics(sq, bishop_mask, sliding_attacks + offset,
                             BISHOP_DELTAS),
        .mask = bishop_mask,
        .offset = offset,
        .shift = 64 - bishop_bits,
    };
    offset += 1U << bishop_bits;

    rook_magics[sq] = (magic_t){
        .magic =
            find_magics(sq, rook_mask, sliding_attacks + offset, ROOK_DELTAS),
        .mask = rook_mask,
        .offset = offset,
        .shift = 64 - rook_bits,
    };
    offset += 1U << rook_bits;

    white_pawn_attacks[sq] = gen_jumping_attacks(sq, WPAWN_CAPTURE_DELTAS);
    black_pawn_attacks[sq] = gen_jumping_attacks(sq, BPAWN_CAPTURE_DELTAS);
    knight_attacks[sq] = gen_jumping_attacks(sq, KNIGHT_DELTAS);
    king_attacks[sq] = gen_jumping_attacks(sq, KING_DELTAS);
  }

  printf(
      "// This file is auto-generated by src/bake.c.\n"
      "// It contains precomputed lookup tables for move generation.\n"
      "// Do not modify manually.\n\n");

  printf(
      "#include \"defs.h\"\n"
      "#include \"luts.h\"\n"
      "\n"
      "const bitboard_t WPAWN_ATTACKS_LUT[NR_OF_SQUARES] = {\n");
  for (square_t sq = SQ_A1; sq <= SQ_H8; sq++) {
    printf("    0x%016lXULL,\n", white_pawn_attacks[sq]);
  }
  printf("};\n\n");

  printf("const bitboard_t BPAWN_ATTACKS_LUT[NR_OF_SQUARES] = {\n");
  for (square_t sq = SQ_A1; sq <= SQ_H8; sq++) {
    printf("    0x%016lXULL,\n", black_pawn_attacks[sq]);
  }
  printf("};\n\n");

  printf("const bitboard_t KNIGHT_ATTACKS_LUT[NR_OF_SQUARES] = {\n");
  for (square_t sq = SQ_A1; sq <= SQ_H8; sq++) {
    printf("    0x%016lXULL,\n", knight_attacks[sq]);
  }
  printf("};\n\n");

  printf("const bitboard_t SLIDING_ATTACKS_LUT[%zu] = {\n", offset);
  for (size_t i = 0; i < offset; i++) {
    printf("    0x%016lXULL,\n", sliding_attacks[i]);
  }
  printf("};\n\n");
  free(sliding_attacks);
  sliding_attacks = NULL;

  printf("const bitboard_t KING_ATTACKS_LUT[NR_OF_SQUARES] = {\n");
  for (square_t sq = SQ_A1; sq <= SQ_H8; sq++) {
    printf("    0x%016lXULL,\n", king_attacks[sq]);
  }
  printf("};\n\n");

  printf("const magic_t BISHOP_MAGICS[NR_OF_SQUARES] = {\n");
  for (square_t sq = 0; sq < NR_OF_SQUARES; sq++) {
    magic_t m = bishop_magics[sq];
    printf("    { 0x%016lXULL, 0x%016lXULL, %u, %u },\n", m.magic, m.mask,
           m.offset, m.shift);
  }
  printf("};\n\n");

  printf("const magic_t ROOK_MAGICS[NR_OF_SQUARES] = {\n");
  for (square_t sq = 0; sq < NR_OF_SQUARES; sq++) {
    magic_t m = rook_magics[sq];
    printf("    { 0x%016lXULL, 0x%016lXULL, %u, %u },\n", m.magic, m.mask,
           m.offset, m.shift);
  }
  printf("};\n\n");

  return 0;
}
