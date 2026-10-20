# Neptune

Neptune is a [UCI](https://en.wikipedia.org/wiki/Universal_Chess_Interface) chess engine written in C++: alpha-beta search with a transposition table and a hand-tuned evaluation. Move generation is verified against the published perft suite and computes about 24 million leaf positions per second on a single core of an Apple M3 Max.

`go depth N` looks a fixed number of plies (half-moves) ahead, and a clocked `go` keeps searching until its time runs out. The search walks the tree of legal moves with alpha-beta pruning, tries captures and previously good moves first. At the horizon it keeps searching captures, answers a check, and plays a promotion so it does not stop halfway through a trade. A transposition table remembers positions already searched. The score at a leaf is based on material, pawn structure, king safety, and the bishop pair. A threefold repetition, fifty moves with no pawn move or capture, and a position where mate is impossible are draws. Checkmate still wins when the fifty-move count is already full.

## How it works

A GUI or a script talks to Neptune over the UCI protocol. Neptune does not draw a board; it reads a position and calculates the best move.

`go depth N` looks a fixed number of plies (half-moves) ahead, and a clocked `go` keeps searching until its time runs out. The search walks the tree of legal moves with alpha-beta pruning and tries captures and previously good moves first. At the horizon it keeps searching captures, answers a check, and plays a promotion so it does not stop halfway through a trade. A transposition table remembers positions already searched. The score at a leaf is based on material, pawn structure, king safety, and the bishop pair.

## Requirements

A C++17 compiler (GCC or Clang) and `make`. The tests need Python 3.

## Build

```
make
```

The binary is `build/neptune`. `make clean` removes `build/`. `make debug` writes a sanitizer build to `build/debug/neptune`.

## Run

Neptune reads UCI on stdin. A short session:

```
./build/neptune
uci
position startpos moves e2e4
go depth 4
quit
```

A position is either the starting position (`position startpos`) or a [FEN](https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation) string, the usual one-line description of a board (`position fen <fen>`). Moves are long algebraic, such as `e2e4`, with a promotion piece on the end (`e7e8q` or `e7e8Q`). Each of those commands takes an optional `moves` list. Printed moves use a lowercase promotion piece.

`go` searches 8 plies by default. `go depth N` searches N plies. `go infinite` searches until `stop`. A clock (`wtime` and `btime`, with `winc`, `binc`, and `movestogo`) or `movetime` searches until the time for the move runs out. After each finished depth Neptune prints an `info` line: depth, a score in centipawns (or `mate N` when the line is a forced mate), the node count, the time in milliseconds, and the line it expects (the principal variation). Then it prints `bestmove`. A position with no legal move prints `bestmove (none)`.

Supported commands are `uci`, `isready`, `ucinewgame`, `setoption`, `position`, `go`, `stop`, and `quit`. `Hash` is the transposition table in megabytes (default 64), `Threads` is 1, and `EvalFile` remembers a weight-file path while the hand-written eval still scores the leaf.

Debug commands, used by the tests:

- `_perft <depth> <fen>` prints how many leaf positions the move generator reaches, and the milliseconds that took
- `_eval <fen>` prints the static score
- `_king <w|b> <fraction> <fen>` prints king safety for that side
- `_eg <fen>` prints how close the position is to an endgame, from 0 to 1
- `_hash` prints the position hash and the pawn-king hash
- `_tt` prints how many entries the transposition table has
- `_option <name>` prints the stored value of `Hash`, `Threads`, or `EvalFile`
- `_roundtrip` checks that making a move and unmaking it restores the hash

## Test

```
make test
make test-perft
```

`make test` checks that known positions still return the same move, and that a shallow move-generation count matches the published number. `make test-perft` runs the full [perft](https://www.chessprogramming.org/Perft) suite: it counts every leaf of the move tree and compares those totals to published numbers. Both runs also count a position with a second rook on the a-file, which the published suite never reaches. This catches move-generation bugs and doubles as a benchmark. The tests are written in Python; set `NEPTUNE` to point them at a different binary.

## Estimating strength

```
make elo
```

`make elo` plays 16 games against [Stockfish](https://stockfishchess.org/download/) set to Elo 2200 and prints the gap. Elo is the standard chess rating; a score near even means the cap was about right, and `ELO` selects another one. [cutechess-cli](https://github.com/cutechess/cutechess) runs the match. On macOS, `brew install stockfish`. The first run downloads a set of opening positions. The time control and the other match settings are in `scripts/elo.sh`.

## History

Neptune is a C++ port of [Tactician](https://github.com/philleski/tactician), a chess engine I wrote in Java. I started the port by hand and finished most of it recently with AI-assisted development.

## License

[MIT](LICENSE.md)
