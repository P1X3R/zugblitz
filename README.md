# Zugblitz

Zugblitz is a UCI-compatible chess engine written in C (because it's simple enough to fit in my brain).

## Why?

I started this project after watching Rey Enigma’s video about the match between Deep Blue and Garry Kasparov.
Even though I barely understood what was happening due to my limited chess knowledge at the time, I became curious about how modern chess engines like Stockfish work, and how it’s possible to build something that can outperform a human at such an intellectually demanding game.

Along the way I learned a lot, and after countless hours debugging perft, I can confidently say that I no longer miss en passant captures.

## Building

Requirements:
- Make
- C11-compatible compiler (clang is hardcoded in the makefile)

```sh
make CC=gcc MODE=release
```

This will build a release-optimized binary for your **specific** platform.

> [!WARNING]
> `debug` mode doesn't work with MinGW due to sanitizers.

## Features

- **Full move generation**: en passant, castling, promotions  
- **Search algorithms**: Alpha-Beta, PVS, quiescence search, null-move pruning  
- **Move ordering heuristics**: killer moves, history heuristics  
- **Evaluation**: incremental midgame/endgame evaluation with PSQTs tuned via Texel’s method  
- **Optimizations**: transposition tables, Zobrist hashing, LTO for release builds  

## Development

This project uses Git Cliff for changelog generation and follows Conventional Commits.

## License

This project is licensed under the MIT License. See the LICENSE file for details.
