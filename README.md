# Zugblitz

Zugblitz is a UCI-compatible chess engine written in C11.

## Why?

I started this project after watching Rey Enigma’s video about the match between Deep Blue and Garry Kasparov. Even though I barely understood what is happening due to my limited knowledge of chess, I became curious about how modern chess engines like Stockfish work, and how the computer outperforms its creators in a game requiring creativity and tactical knowledge.

Along the way I learned a lot, and after countless hours debugging perft, I can confidently say that I no longer miss en passant captures.

## Features

- **Full move generation**: en passant, castling, promotions  
- **Search algorithms**: Alpha-Beta, PVS, quiescence search, null-move pruning  
- **Move ordering heuristics**: killer moves, history heuristics  
- **Evaluation**: incremental midgame/endgame evaluation with PSQTs tuned via Texel’s method (see [Zugblitz Texel's tuning pipeline](https://github.com/P1X3R/texel_zugblitz))
- **Optimizations**: transposition tables, Zobrist hashing, LTO for release builds  

## Architecture

```mermaid
flowchart LR
    Position --> MoveGen["Pseudo-Legal Move Generation"]
    MoveGen --> Legality["Legality Checking"]
    Legality --> Search
    Search --> Evaluation
    Evaluation --> BestMove["Best Move"]
    BestMove --> UCI["UCI Response"]
```

The engine generates pseudo-legal moves, filters illegal ones through king-safety checks, searches the resulting game tree, evaluates leaf positions, and returns the best move through the UCI interface.

## Results

### Strength 

Around **2200 Elo** estimated by playing against Petrel.

| Opponent | Published Rating | Estimated Result |
|-----------|-----------|-----------|
| Petrel 1.0 | 2000 CCRL Blitz | +79 ± 48 Elo |
| Petrel 2.1 | 2624 CCRL Blitz | -436 ± 126 Elo |

- **Time control:** 10+0.1
- **Games:** ~100 per opponent

According to [Petrel](https://github.com/AleksPeshkov/petrel) author, Aleks Peshkov, these results place Zugblitz around 2200 CCRL ELO at LTC.

### Speed

| Benchmark | N/s |
| -------------- | --------------- |
| Perft (no hash) | 14.7M - 15.5M |
| Search (32MB TT size) | ~4.7M |


- **CPU:** Intel Pentium Silver N5030
- **RAM:** 4GB DDR4

## Building

Requirements:
- Make
- C11-compatible compiler (clang is the default in the makefile)

```sh
make CC=gcc MODE=release
```

This will build a release-optimized binary for your **specific** platform.

> [!WARNING]
> `debug` mode doesn't work with MinGW due to sanitizers.

## Interesting challenges

### Legal move debugging

A significant portion of the project was spent debugging legal move generation. The search quickly finds weird edge-cases where the move generator fails; therefore, it is critical to have a highly reliable legal move generator in order to maintain playing strength. This is achieved through Perft, the de facto standard test to find bugs in move generators.

### Testing methodologies

As stated in the previous point, testing is even more important to ensure correctness in a chess engine's source code, and this remains true beyond legal moves. Initially, I wasn't really testing features so I was building buggy features upon buggy features, consequently, previous prototypes were significantly weaker even though they had more features.

This continued until the project began to use SPRT to test every feature by running matches against a previous version of itself and see if it's actually stronger or weaker.

### Evaluation tuning

Evaluation parameters (PSQTs) are fine tuned through Texel's Tuning method. However, the traditional stochastic hill climbing is very slow, especially with my machine's hardware constraints, so I decided to use a gradient-based approach with PyTorch in [Zugblitz Texel's tuning pipeline](https://www.github.com/P1X3R/texel_zugblitz).

## License

This project is licensed under the MIT License. See the LICENSE file for details.
