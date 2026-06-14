# Zugblitz's Design Document

## Core Architecture

The engine follows a traditional alpha-beta architecture:

The search algorithm drives the engine. During search, moves are generated,
positions are evaluated, and the best continuation is selected.

## Board representation

Zugblitz uses standard bitboards alongside a redundant piece-lists, both piece-centric board representations. 

Bitboards are used for attack generation and occupancy queries, while piece lists allow fast piece-type querying.

This is a well-stablished board structure in modern chess engines.
## Move representation

The engine represents moves internally as 16-bit numbers with the following encoding:

| Bits | Encoded info |
| -------------- | --------------- |
| 0-5 | From square |
| 6-11 | To square |
| 12-15 | Flags |


I decided to use this move representation becuase it's extremely compact and allows smaller TT entries, this means that more entries fit in the same memory space!

## Move Generation

The move generation process is divided in two:

1. **Pseudo-legal move generator**: Generates the moves for each piece of the board based on their general moving rules
2. **Legal-move checking:** enforces king safety for each move only when needed

This implementation is significantly simpler than generating legal moves directly and has the potential of becoming extremely fast. However, Zugblitz uses Ethereal's approach: which simply check if the king is being attacked by a piece one by one instead of using attack tables, this approach generates moves fast enough to be competitive.

## Search

The search is built around Alpha-Beta pruning, although it includes many techniques like:

### Iterative deepening

Search starts at one and increases depth by 1 restarting the search each time. This may seem a bit redundant but it allows:

1. Better move ordering
2. Better TT population
3. Principal variation extraction
4. Stabler time management
5. Simplified UCI logging logic

### Principal Variation Search (PVS)

Searches the first moves with a full-window until it finds a move that improves alpha and the rest with a null-window. This reduces node count while preserving correctness.

### Quiescence search

Instead of immediately returning the evaluation of leaf nodes, Zugblitz does a search only over captures to avoid "horizon effect" by only evaluating until it finds a stable position.

### Null-move pruning (NMP)

Null-move pruning is used to aggressively cut branches that are unlikely to affect the final evaluation.

## Move ordering

Alpha-Beta pruning is more effective when the best moves are searched first, specially when paired with PVS.

Zugblitz uses many heuristics (listed by priority order):

1. **TT-move**: If the move proved to be good previously in the same position, it's likely to be good again.
2. **MVV-LVA**: A capture is prioritized by victim-attacker value.
3. **Killer moves**: Quiet moves that caused beta-cutoffs previously are prioritized in future search.
4. **History heuristics**: Quiet moves accumulate bonuses and penalties depending on search outcomes. This information is used to improve future move ordering.

## Evaluation

The evaluation function consists of Piece-Square Tables with tapering. This means the score is divided in midgame and endgame score and they're blended on the position's game phase.

The use of tapered evaluation allows pieces to change value naturally between the opening and endgame.

### Evaluation Tuning

PSQT values are not tuned manually.

Instead, Zugblitz uses a separate [Texel-style tuning pipeline](https://github.com/P1X3R/texel_zugblitz) based on PyTorch.

The pipeline learns:

- Midgame PSQTs
- Endgame PSQTs
- Logistic scaling parameter (The parameter that converts centipawns to winning probability)

The resulting parameters are exported directly into C source code.

A gradient-based optimization approach was chosen over traditional hill-climbing techniques due to significantly faster convergence on the available hardware.

## Transposition tables

Zugbltz uses Zobrist hashing to quickly index positions into a hashmap-like structure called Transposition tables (TT).

Each TT entry stores:
- Position hash (key)
- Search score
- Best move
- Bound type (Lower, Upper, Exact)
- Search depth

This allows memoization to improve search performance and improves move ordering.

## Time management

The allocated time for search is based directly on UCI inputs. The engine uses:

- Soft time limit (the desired finishing time)
- Hard time limit (the limit finishing time)
- Overhead compensation

This allows search to terminate safely before the clock expires.

## Testing

### Move generation

Move generation is validated through **Perft** (you can use the `go perft` command just like in Stockfish).

Perft is used extensively to find edge-cases where the engine fails to generate all legal moves in a position. Most bugs are king-safety related or en-passant handling.

### Playing strength

Each change must pass through a regression test, which validates if the change did actually improve the engine or not. This is done through **SPRT**, where the engien plays against previous versions and estimates whether the observed Elo gain is statistically significant.

A major lesson learned during development was that additional features do not necessarily increase playing strength unless they are measured objectively.

## Future Work

Potential future improvements include:

### Likely

- Aspiration windows
- LMR

### Experimental

- NNUE
- Syzygy

## Tradeoffs

### Why C11?

C11 provides low-level control and predictable performance while being an extremely simple language in comparison to others like C++.

### Why handcrafted evaluation?

Becuase it's extremely simple to optimize under hardware constraints in comparison to other architectures like NNUE.
