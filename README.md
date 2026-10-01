# Neptune

Neptune is a chess engine. A GUI or a script talks to it over the [UCI](https://en.wikipedia.org/wiki/Universal_Chess_Interface) protocol. Neptune does not draw a board; it reads a position and calculates the best move.

The search looks a fixed number of plies (half-moves) ahead. It walks the tree of legal moves with alpha-beta pruning, tries captures and previously good moves first. At the horizon it keeps searching captures so it does not stop halfway through a trade. A transposition table remembers positions already searched. The score at a leaf is based on material, pawn structure, king safety, and the bishop pair.

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

`go` searches 8 plies by default. `go depth N` searches N plies. Neptune then prints an `info` line: depth, a score in centipawns, the gameplay it expects, and then `bestmove`.

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

`make test` checks that known positions still return the same move, and that a shallow move-generation count matches the published number. `make test-perft` runs the full [perft](https://www.chessprogramming.org/Perft) suite: it counts every leaf of the move tree and compares those totals to published numbers. The purpose is to check for any bugs in move generation, and to act as a performance test. Set `NEPTUNE` to point the Python tests at a different binary.

## License

[MIT](LICENSE.md)
