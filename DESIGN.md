# Zugblitz's Design Document

## Core Architecture

The engine follows a traditional alpha-beta architecture:

The search algorithm drives the engine. During search, moves are generated, positions are evaluated, and the best continuation is selected.

## Board Representation

Zugblitz uses standard bitboards alongside a redundant piece list, both piece-centric board representations.

Bitboards are used for attack generation and occupancy queries, while piece lists allow fast piece-type querying.

This is a well-established board structure in modern chess engines.

## Move Representation

The engine represents moves internally as 16-bit numbers with the following encoding:

| Bits  | Encoded info |
| ----- | ------------ |
| 0-5   | From square  |
| 6-11  | To square    |
| 12-15 | Flags        |

I decided to use a manually packed representation rather than just a struct with bitfields to ensure total control and allow bitwise logic over the whole move in memory.

## Move Generation

The move generation process is divided into two:

1. **Pseudo-legal move generator**: Generates the moves for each piece on the board based on their general moving rules
2. **Legal move checking:** Enforces king safety for each move only when needed

This implementation is significantly simpler than generating legal moves directly and has the potential to become extremely fast. However, Zugblitz uses Ethereal's approach, which simply checks whether the king is being attacked by a piece one by one instead of using attack tables. This approach generates moves fast enough to be competitive.

Sliding pieces are a special case in pseudo-legal move generation because they need occupancy checking. To solve this, instead of calculating ray attacks each time, Zugblitz uses Fancy Magic Bitboards, which take only a couple of CPU cycles.

## Search

The search is built around Alpha-Beta pruning, although it includes many techniques such as:

### Iterative Deepening

Search starts at depth one and increases depth by 1, restarting the search each time. This may seem a bit redundant, but it allows:

1. Better move ordering
2. Better TT population
3. Principal variation extraction
4. More stable time management
5. Simplified UCI logging logic

### Principal Variation Search (PVS)

Searches the first move with a full window until it finds a move that improves alpha, and the rest with a null window. This reduces node count while preserving correctness.

### Quiescence Search

Instead of immediately returning the evaluation of leaf nodes, Zugblitz performs a search only over captures to avoid the horizon effect by evaluating only until it finds a stable position.

### Null-Move Pruning (NMP)

Null-move pruning is used to aggressively cut branches that are unlikely to affect the final evaluation.

## Move Ordering

Alpha-Beta pruning is more effective when the best moves are searched first, especially when paired with PVS.

Zugblitz uses many heuristics (listed in priority order):

1. **TT-move**: If the move proved to be good previously in the same position, it's likely to be good again.
2. **MVV-LVA**: A capture is prioritized by victim-attacker value.
3. **Killer moves**: Quiet moves that caused beta cutoffs previously are prioritized in future searches.
4. **History heuristics**: Quiet moves accumulate bonuses and penalties depending on search outcomes. This information is used to improve future move ordering.

## Evaluation

The evaluation function consists of Piece-Square Tables with tapering. This means the score is divided into a midgame and endgame score, and they are blended based on the position's game phase.

The use of tapered evaluation allows pieces to change value naturally between the opening and endgame.

### Evaluation Tuning

PSQT values are not tuned manually.

Instead, Zugblitz uses a separate Texel-style tuning pipeline based on PyTorch.

The pipeline learns:

* Midgame PSQTs
* Endgame PSQTs
* Logistic scaling parameter (the parameter that converts centipawns to winning probability)

The resulting parameters are exported directly into C source code.

A gradient-based optimization approach was chosen over traditional hill-climbing techniques due to significantly faster convergence on the available hardware.

## Transposition Tables

Zugblitz uses Zobrist hashing to quickly index positions into a hashmap-like structure called a Transposition Table (TT).

Each TT entry stores:

* Position hash (key)
* Search score
* Best move
* Bound type (Lower, Upper, Exact)
* Search depth

This allows memoization to improve search performance and move ordering.

## Time Management

The allocated time for search is based directly on UCI inputs. The engine uses:

* Soft time limit (the desired finishing time)
* Hard time limit (the limit finishing time)
* Overhead compensation

This allows search to terminate safely before the clock expires.

## Testing

### Move Generation

Move generation is validated through **Perft** (you can use the `go perft` command just like in Stockfish).

Perft is used extensively to find edge cases where the engine fails to generate all legal moves in a position. Most bugs are king-safety related or involve en passant handling.

### Playing Strength

Each change must pass through a regression test, which validates whether the change did actually improve the engine or not. This is done through **SPRT**, where the engine plays against previous versions and estimates whether the observed Elo gain is statistically significant.

A major lesson learned during development was that additional features do not necessarily increase playing strength unless they are measured objectively.

## Future Work

Potential future improvements include:

### Likely

* Aspiration windows
* LMR

### Experimental

* NNUE
* Syzygy

## Tradeoffs

### Why C11?

C11 provides low-level control and predictable performance while being an extremely simple language in comparison to others like C++.

### Why handcrafted evaluation?

Because it's extremely simple to optimize under hardware constraints in comparison to other architectures like NNUE.
