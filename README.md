# Neptune

Neptune is a [UCI](https://en.wikipedia.org/wiki/Universal_Chess_Interface) chess engine written in C++: alpha-beta search with a transposition table and a hand-tuned evaluation. Move generation is verified against the published perft suite and computes about 24 million leaf positions per second.

## How it works

A GUI or a script talks to Neptune over the UCI protocol. Neptune does not draw a board; it reads a position and calculates the best move.

The search looks a fixed number of plies (half-moves) ahead. It walks the tree of legal moves with alpha-beta pruning and tries captures and previously good moves first. At the horizon it keeps searching captures so it does not stop halfway through a trade. A transposition table remembers positions already searched. The score at a leaf is based on material, pawn structure, king safety, and the bishop pair.

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

A position is either the starting position (`position startpos`) or a [FEN](https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation) string, the usual one-line description of a board (`position fen <fen>`). Moves are long algebraic, such as `e2e4`, with a promotion piece on the end (`e7e8q`). Each of those commands takes an optional `moves` list.

`go` searches 8 plies by default. `go depth N` searches N plies. Neptune then prints an `info` line: depth, a score in centipawns, the line it expects (the principal variation), and then `bestmove`.

Supported commands are `uci`, `isready`, `ucinewgame`, `position`, `go`, and `quit`.

Debug commands, used by the tests:

- `_perft <depth> <fen>` prints how many leaf positions the move generator reaches, and the milliseconds that took
- `_eval <fen>` prints the static score
- `_king <w|b> <fraction> <fen>` prints king safety for that side
- `_eg <fen>` prints how close the position is to an endgame, from 0 to 1
- `_hash` prints the position hash and the pawn-king hash
- `_roundtrip` checks that making a move and unmaking it restores the hash

## Test

```
make test
make test-perft
```

`make test` checks that known positions still return the same move, and that a shallow move-generation count matches the published number. `make test-perft` runs the full [perft](https://www.chessprogramming.org/Perft) suite: it counts every leaf of the move tree and compares those totals to published numbers. This catches move-generation bugs and doubles as a benchmark. The tests are written in Python; set `NEPTUNE` to point them at a different binary.

## History

Neptune is a C++ port of [Tactician](https://github.com/philleski/tactician), a chess engine I wrote in Java. I started the port by hand and finished most of it recently with AI-assisted development.

## License

[MIT](LICENSE.md)
