# 0009. Terminal bestmove and promotion case

Date: 2026-10-20

## Status

Accepted

## Context

Two UCI tokens are wrong. A third report, a short FEN leaving `ply` uninitialized, does not reproduce.

### No legal move

A root with no legal move already has a mate or draw score. [0004](0004-quiescence-horizon.md) made `getMoveToDepth` set that score, and [0005](0005-search-scores.md) prints it as `score mate 0` when the side to move is checkmated. `getMove` copies the score before it stops on the missing move.

```444:447:src/brain.cpp
	if(count == 0) {
		result.score = moveGen.inCheck(board) ? -MATE : 0;
		return result;
	}
```

```547:553:src/brain.cpp
		result.move = found.move;
		result.score = found.score;
		result.depth = depth;
		emitInfo(result);
		if(found.move == MOVE_NONE) {
			break;
		}
```

The printed move is still the null-move token. `MOVE_NONE` is 0, and `moveToUci` turns that into `0000`.

```42:45:src/neptune.cpp
std::string moveToUci(Move move) {
	if(move == MOVE_NONE) {
		return "0000";
	}
```

Measured on `build/neptune`, each of these is one info line and then `bestmove 0000`:

- `R6k/8/6K1/8/8/8/8/8 b - - 0 1` at `go depth 4`: `info depth 1 score mate 0 nodes 0 time 0 pv`
- After `f2f3 e7e5 g2g4 d8h4`, `go depth 3`: the same mate line
- `k7/P7/1K6/8/8/8/8/8 b - - 0 1` at `go depth 4`: `info depth 1 score cp 0 nodes 0 time 0 pv`

The mate score is the checkmate. The centipawn 0 is the stalemate. Neither is the initializer left behind by an abort. `test/engine.py` locks the token: `already checkmated` and `root checkmate` expect `0000` with mate 0, and `root stalemate` expects `0000` with score 0.

`0000` is a null move. A GUI that ends the game on `bestmove (none)` does not end it on `0000`, and a match runner that treats `0000` as a played move forfeits the engine. Neptune has no null move to send. The principal variation already stops on `MOVE_NONE`, so the info line's `pv` is empty.

A repetition or a dead position still has a move. [0003](0003-draw-rules.md) returns that move and score 0 without searching. King versus king at depth 1 prints `bestmove e5d4`. That token stays a move.

### Promotion case

`findUciMove` compares the fifth character to a lowercase piece. `e7e8q` matches a queen. `e7e8Q` matches nothing, the function returns `MOVE_NONE`, and the `moves` list stops.

```87:103:src/neptune.cpp
		if(promotion) {
			if(!IS_PROMOTION(*current)) {
				continue;
			}
			int piece = *current & 0x3000;
			if(promotion == 'q' && piece == MOVE_PROMOTE_TO_QUEEN) {
				return *current;
			}
			if(promotion == 'r' && piece == MOVE_PROMOTE_TO_ROOK) {
				return *current;
			}
			if(promotion == 'b' && piece == MOVE_PROMOTE_TO_BISHOP) {
				return *current;
			}
			if(promotion == 'n' && piece == MOVE_PROMOTE_TO_KNIGHT) {
```

Measured by `_hash` after the move, against the hash of the position the move should produce. From `k7/4P3/8/8/8/8/8/4K3 w - - 0 1`:

- `e7e8Q`, `e7e8R`, `e7e8B`, and `e7e8N` each leave the hash of the unmoved position.
- `e7e8q` matches `k3Q3/8/8/8/8/8/8/4K3 b - - 0 1`. `e7e8r`, `e7e8b`, and `e7e8n` match the rook, bishop, and knight on e8.
- `a7a8Q` from `4k3/P7/8/8/8/8/8/4K3 w - - 0 1` does not match `a7a8q`, which matches `Q3k3/8/8/8/8/8/8/4K3 b - - 0 1`.
- `e7d8Q` from `k2r4/4P3/8/8/8/8/8/4K3 w - - 0 1` does not match `e7d8q`, which matches `k2Q4/8/8/8/8/8/8/4K3 b - - 0 1`.
- `e2e1Q` from `4k3/8/8/8/8/8/4p3/7K b - - 0 1` leaves the unmoved hash. `e2e1q` matches `4k3/8/8/8/8/8/8/4q2K w - - 0 2`.

The engine already prints a lowercase piece. The existing `a7a8q` hash test covers that direction only.

### Short FENs

A FEN with no halfmove and no fullmove does not leave `ply` uninitialized. `halfmove` starts at 0 and `fullmove` starts at 1. A failed read of the fullmove is clamped back to 1, and `ply` is assigned from that value. `clear` also sets `ply` to 0 before the parse.

```167:178:src/board.cpp
	// Steps 5-6: Halfmove clock and fullmove counter.
	// A missing field fails the read. The clock then stays 0, and fullmove stays 1.
	ss >> std::skipws >> halfmove >> fullmove;
	if(halfmove < 0) {
		halfmove = 0;
	}
	if(fullmove < 1) {
		fullmove = 1;
	}
	ply = 2 * (fullmove - 1) + (turn == BLACK);
	historyPly = ply;
	halfmoveClock = halfmove;
```

`_eval` of a missing king adds `ply * FITNESS_MOVE` (10000). A four-field FEN matches the same placement with `0 1`, and both differ from fullmove 40 by the ply term:

- `8/8/8/4k3/8/8/8/8 w - -` and `... w - - 0 1` both print `-1.00016e+06`. `... w - - 0 40` prints `-220162`.
- `8/8/8/4K3/8/8/8/8 b - -` and `... b - - 0 1` both print `-990192`. `... b - - 0 40` prints `-210192`.

White at fullmove 1 is ply 0. Black at fullmove 1 is ply 1. Fullmove 40 is 78 plies later. The black scores differ by 780000, which is 78 times the ply term, and the white scores differ by that same step. `undoHistory[ply]` is inside the array.

The same default holds for the rest of the board. The four-field start position hashes equal to `0 1`, `_roundtrip` prints `ok`, and `moves e2e4` hashes equal to `rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1`. A four-field en passant field does too: `... b KQkq e3` matches `... e3 0 1`. A halfmove with no fullmove, `... w - - 12`, evals equal to `... 12 1`. Queen versus king at `... w - - 100` searches the same depth-2 draw as `... 100 60` (`bestmove a2b1`, score 0) and does not match clock 0 (`bestmove a1b2`, score 790).

This is the class of bug [0002](0002-quality-pass.md) fixed for castling rights, and the clock path is already initialized. `setPosition` stays as it is.

## Decision

Print `bestmove (none)` when the engine has no move, and accept an uppercase promotion letter as the same piece as the lowercase one.

### bestmove

`moveToUci(MOVE_NONE)` returns `(none)`. The `bestmove` line uses that string. The info line is unchanged: checkmate stays `score mate 0`, stalemate stays `score cp 0`, and the `pv` stays empty. The search loop is unchanged. A root that has a legal move still prints that move, including the move [0003](0003-draw-rules.md) returns for a repetition or a dead position.

### Promotion

The fifth character is matched without regard to case. `Q` and `q` are a queen, `R` and `r` a rook, `B` and `b` a bishop, `N` and `n` a knight. Any other fifth character still matches nothing, and the `moves` list still stops there. Moves the engine prints stay lowercase.

### Tests

`test/engine.py` expects `(none)` where it now expects `0000`:

- `already checkmated` and `root checkmate` stay mate 0, with bestmove `(none)`.
- `root stalemate` stays score 0, with bestmove `(none)`.

`expect_some_move`, and `expect_scored` when it requires a move, treat `(none)` as no move.

Promotion hashes, each uppercase move against the lowercase move and against the FEN of the resulting piece:

- `k7/4P3/8/8/8/8/8/4K3 w - - 0 1` with `e7e8Q`, `e7e8R`, `e7e8B`, `e7e8N`.
- `e7d8Q` from `k2r4/4P3/8/8/8/8/8/4K3 w - - 0 1`.
- `e2e1Q` from `4k3/8/8/8/8/8/4p3/7K b - - 0 1`.
- The existing `a7a8q` check stays, and `a7a8Q` matches it.

Mate in one, the fastest mate, and the fork stay on their moves. Perft is unchanged.

### README

The moves sentence says a promotion piece may be `e7e8q` or `e7e8Q`. The `go` sentence says a position with no legal move prints `bestmove (none)` after the info line. Printed moves stay lowercase.

### Out of scope

- Mate and stalemate scores, and the info-line fields around them.
- `Board::setPosition`, including a FEN that omits the clocks.
- Reading `0000` or `(none)` from a `moves` list.
- Uppercase square letters, as in `E7E8q`.
- A null move in the search.
- The move returned for a repetition, a dead position, or a fifty-move root.

## Consequences

- A finished game is `bestmove (none)` with the mate or draw score already on the info line. A GUI can end the game instead of playing `0000`.
- `e7e8Q` plays the same move as `e7e8q`. A list that used to stop on the uppercase letter now plays it and continues.
- Search choices at a fixed depth stay put. The token on a terminal root is the only `bestmove` that changes.
