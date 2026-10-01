# 0002. Quality pass without changing play

Date: 2026-10-01

## Status

Accepted

## Context

[0001](0001-port-tactician-engine.md) made Neptune a UCI engine: Tactician’s search, evaluation, and hashing on the existing make/unmove board. The sources are a flat `src/` of about fifteen files, tests are two Python scripts run by hand, and the README still describes a command-line toy. The default branch is `master`.

A later phase will add clocks, an opening book, and endgame tablebases. After that, training will live in Python and inference in C++. Neither is this change. This change makes the current engine easier to trust and change without moving its play.

Search and evaluation are behavior-sensitive. Weights, quiescence (recaptures on the same square only), killer and transposition-table move ordering, the always-replace table, the Zobrist seed, and mate scoring (`FITNESS_LARGE` and the ply penalty) all affect which move comes back at a fixed depth. Renaming `fitness` to `score`, splitting `src/` into subfolders, or reformatting the tree would obscure the Java port without making the engine more correct.

A few defects are not search decisions:

- The UCI loop keeps the previous token. A blank line or EOF fails to read a new token and replays the last command, so a piped `go` can run forever.
- `uciok` and `readyok` are not flushed. On a pipe, stdout is fully buffered and a GUI can hang on the handshake.
- `Board` ORs castling rights into an uninitialized `unsigned int`. The low four bits end up correct for legal FENs, and the hash masks with `& 15`, but the read is undefined behavior.
- Pawn attack tables are filled for the first and eighth ranks. A one-step walk from those squares reads off the end of `BB_SET`. Pawns promote before they can stand there, so the tables for ranks 2–7, which are the ones search uses, stay the same.

Transposition-table scores are raw floats, and mate scores are stored without a ply adjustment. Correcting that changes search results, so it is a known limitation, not a fix in this pass.

## Decision

Lock current play with tests, then fix robustness and names that are wrong. Do not retune search or evaluation.

### In scope

- Rewrite the README: UCI fixed-depth engine, build, a sample position, how to test, the debug commands (`_perft`, `_eval`, `_king`, `_eg`, `_hash`, `_roundtrip`), a pointer to the ADRs and the MIT license, and a short list of what is not built yet (clocks, books, tablebases, a network, and the float mate-score table).
- Keep `test/perft.py` and `test/engine.py`. Share a small driver: binary path from the script location, overridable with `NEPTUNE`, shebang `#!/usr/bin/env python3`, one process per script. The default binary is `build/neptune`. Add tests of behavior that already exists: the UCI handshake, castling and promotion in a `moves` list, the `info` line before `bestmove`, bishop-pair and rook-file orderings, and the three tactical cases at depth 6. Keep the “any legal move” positions at depth 3. Default `make test` runs the engine tests plus a shallow perft. The full perft suite stays a separate slow target.
- Move the Makefile to the repo root and write every build product under `build/`, which is gitignored. `src/` stays sources only. The release binary is `build/neptune` with its objects in `build/`. The debug binary is `build/debug/neptune` with its own objects, so the two builds do not clobber each other. `bin/` is the wrong place: that name is for checked-in or installed executables, and object files do not belong there. The repo root is the wrong place too: it holds the README, license, and docs, and a binary there is still an artifact with nowhere natural for the objects. `clean` removes `build/`. Flags stay `CXX` / `CXXFLAGS` (default `g++ -std=c++17 -Wall -Wextra -O3`), plus `test`, `test-perft`, and `debug` (`-O0 -g -fsanitize=address,undefined`). Fix `-Wextra` findings only when the fix does not change results.
- Fix the defects above. Protocol text of a normal session stays the same. Zero `castleRights` before setting it; do not invent a new hash or a new castle decision. Skip the first and eighth ranks when filling pawn attack tables. An illegal move in a `moves` list still stops the list with no extra `info string`.
- Rename only where the name lies. `legalMovesFast` becomes `generateMoves` (moves are pseudo-legal; `isLegal` is separate). `difftime` becomes `elapsedMillis`. Parenthesize the move macros in `types.h`. Delete unused `printMove`, `Board::print`, and the `FILE` / `RANK` macros, which expand to letters and collide with `FILE*`. The `Board::move` parameter can be named `played`. Add a few comments: pseudo-legal generation, recapture-only quiescence, mate distance via `ply`, and hashes updated in `move` and restored from `Undo`.
- GitHub Actions runs `make test` from the repo root. It does not run full perft.
- After CI is on the branch, rename `master` to `main` and switch the GitHub default. Nothing in the repo is pinned to the old name.

### Out of scope

- Time management, `stop`, books, tablebases, and the neural network.
- Replacing float table scores or adjusting mate scores by ply.
- Renaming `fitness*` to `score`. One sentence on `Evaluation` can say that fitness is the static score.
- New source directories, file renames, `clang-format`, or splitting `neptune.cpp` into a UCI module. `build/` is a generated output directory, not a source layout.
- Sharing the two `MoveGen` objects. Each builds its own magic tables. That is wasteful and deterministic, and not worth an ownership change here.
- Replacing the variable-length arrays in magic generation unless `-Wpedantic` or the sanitizer build rejects them. That code runs only at startup.

### Order

Tests and the Makefile land first, so the bug fixes have a check. Renames and comments follow. README and CI follow those. The branch rename is last, so the workflow file is what GitHub defaults to.

## Consequences

- A valid UCI session still gets the same moves and the same scores. Blank input and EOF stop instead of replaying `go`. GUIs see flushed handshake lines.
- Perft node counts and the existing eval orderings stay the lock. Depth-6 tactical tests lock the moves ADR 0001 promised.
- `fitness` remains the name of the static score, so this tree still matches Tactician’s evaluation by eye.
- The float transposition table, including unadjusted mate scores, stays as it is until a change that is allowed to move the search.
- Clones that track `master` need one fetch and a checkout of `main` after the rename.
- `make` from the repo root produces `build/neptune`. `src/` no longer contains a binary. Ignore `build/` instead of `src/neptune`.
