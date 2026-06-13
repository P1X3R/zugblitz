## [1.3.2] - 2026-06-13

### 🚀 Features

- Add `go perft` command

### 💼 Other

- Improve portability on `portable` mode
- Target x86_64-v2 for generic builds

### 📚 Documentation

- Add a clarification comment at the top of `src/luts.c`
- Document that this project uses git cliff and conventional commits
- Improve README

### ⚙️ Miscellaneous Tasks

- Include dist/ dir on .gitignore
- Improve README.md
- Rename to Zugblitz (Golem was already taken)
- Update CHANGELOG.md
## [1.3.1] - 2026-01-09

### 🚜 Refactor

- Refine quiet move detection

### ⚙️ Miscellaneous Tasks

- Remove ghost main.c file
- Add license
- Update changelog and major version
- Change default MoveOverhead to 100 ms
- Improve Windows compatibility and fix warnings in gcc
## [1.3.0] - 2026-01-08

### 🚀 Features

- *(search)* Add null-move pruning
## [1.2.0] - 2026-01-08

### 🚀 Features

- *(ordering)* Add history heuristics
## [1.1.0] - 2026-01-07

### 🚀 Features

- *(ordering)* Add killer moves
## [1.0.0] - 2026-01-07

### ⚙️ Miscellaneous Tasks

- Add changelog and version to `uci`
## [0.4.1] - 2026-01-07

### 🚀 Features

- *(uci)* Add `setoption`

### ⚙️ Miscellaneous Tasks

- Add a readme
## [0.4.0] - 2026-01-07

### 🚀 Features

- Add PVS
## [0.3.0] - 2026-01-06

### 🚀 Features

- Add transposition tables

### 🐛 Bug Fixes

- Add color for ZOBRIST_PIECES
## [0.2.0] - 2025-12-31

### 🚀 Features

- *(search)* Add quiescence search and move ordering
## [0.1.0] - 2025-12-29

### 🚀 Features

- Add gitignore
- Add attack look-up tables
- Add zobrist tables
- *(board)* Add fen parsing
- *(movegen)* Generate non-evasion moves (pseudo-legal)
- *(board)* Add move legality check
- *(board)* Add move making
- *(movegen)* Add en passant captures generation
- *(board)* Add efficient copy-make
- Add draw detection and history tracking; refactor move generation
- Add incremental evaluation function
- *(search,uci)* Implement alpha-beta search and UCI protocol

### 🐛 Bug Fixes

- *(board)* Update castling rights when a rook captures another rook

### 💼 Other

- Switch to clang and unify CFLAGS for C11

### 🚜 Refactor

- Create helper for random u64
- Fix all compiler warnings
- *(evaluation)* Improve PSQTs
- *(board, eval)* Replace int16_t with int for mg_score and eg_score
- *(core)* Rename rand.h to misc.h and update function signatures

### ⚡ Performance

- Enable LTO on release mode

### 🎨 Styling

- Move evaluation function definition to header file

### ⚙️ Miscellaneous Tasks

- Init
- Add .clang-format
- Add makefile
- Include .cache in .gitignore
