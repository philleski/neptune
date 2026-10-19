# 0004. Checks and promotions at the horizon

Date: 2026-10-19

## Status

Accepted

## Context

At depth 0, `alphabeta` calls `quiescentSearch` and returns that score. Quiescence always stand-pats with `Evaluation::fitness`, then generates legal captures, then keeps only recaptures on the square of the capture just played. `target == -1` on the first call accepts any capture. A later call passes the destination and skips every other square.

A stand-pat is a legal score only when the side to move is allowed to do nothing. In check, a move is forced. The static score of that position is the score of an illegal pass. Checkmate and stalemate are scored in `alphabeta` only when `depth > 0` and the legal list is empty: a loss if `inCheck`, and 0 otherwise. The horizon never asks.

There is no neural evaluation. `fitness` is the hand-written static score, and [0001](0001-port-tactician-engine.md) is where a network would replace that function. The network would have the same hole. It would be asked for a static score of a position where a move is forced. The hole is the stand-pat, not the formula inside `fitness`.

Two existing paths already return a mate score at depth 0, and both are narrower than a check:

- A move that captures the king leaves that bitboard empty. Quiescence and `alphabeta` score a missing king as `-FITNESS_LARGE + ply * FITNESS_MOVE`. A mating net that leaves the king on the board does not.
- When the halfmove clock is already at 100 and the horizon side is in check, depth 0 generates every legal move and, if the list is empty, returns that same loss. [0003](0003-draw-rules.md) added this so a fifty-move claim cannot hide a mate. It does not search a quiet evasion. If any legal move exists, the claim path falls through and stand-pats.

Non-capturing promotions are quiet moves. `appendMovesForPawn` skips the pawn-push loop when `capturesOnly` is set, and that loop is where a pawn on the seventh rank promotes. Capturing promotions and en passant are already in the capture loop. A pawn one step from a quiet promotion is still a pawn in the leaf eval.

Measured on the current binary:

- After `f2f3 e7e5 g2g4`, depth 1 plays `d7d5` and scores 70. Depth 2 plays `d8h4` and scores 999960000. `Qh4` is mate and does not capture the king. Depth 1 stand-pats on the mated side, whose static score is about -60, and prefers a pawn push.
- `7k/8/6K1/8/8/8/8/R7 w - - 0 1` at depth 1 plays `g6f7` and scores 707. Depth 2 plays `a1a8` and scores a mate. The same position with the clock at 100 plays `a1a8` already at depth 1, because of the fifty-move exception above. The mate did not get more visible. The claim path noticed an empty evasion list.
- `4k3/P7/8/8/8/8/8/4K3 b - - 0 1` at depth 1 scores -146. Depth 2 scores -866 and the line is `e8d7 a7a8q`. The queen is 880. Depth 1 evaluates the pawn.
- `k7/P7/2K5/8/8/8/8/8 w - - 0 1` at depth 1 plays `c6b6` and scores 315. That king move stalemates Black. The stalemate's static score is about -315 for Black, so White is handed the negation. Every other legal move lets Black take the pawn, after which the position is king versus king and the score is 0. Depth 1 prefers the stalemate because 315 is above 0.
- The position after `d8h4` is checkmate and the root returns `bestmove 0000` with score 0. `getMove` starts the score at 0 and discards the result when `getMoveToDepth` finds no move. `k7/P7/1K6/8/8/8/8/8 b - - 0 1` is stalemate and returns the same `0000` and 0. The draw score is the initializer. The mate score is wrong.

Recapture-only quiescence for ordinary captures stays. [0002](0002-quality-pass.md) treated that limit as part of current play. This change is allowed to move search scores. It is not allowed to retune `fitness`.

## Decision

Quiescence scores a check by searching it, and it scores a quiet promotion as a tactical move. A node with no legal moves is mate or stalemate at the horizon and at the root.

### In check

Do not stand-pat. Do not apply the recapture-square filter. Generate every legal move, the same set `alphabeta` uses, and search every one. The incoming `target` is ignored.

An empty list is checkmate. Return `-FITNESS_LARGE + ply * FITNESS_MOVE`, the score `alphabeta` already uses. Return that score itself, rather than the alpha of a stand-pat window.

A capture passed to the child keeps the recapture chain: the child `target` is the destination. A quiet move did not capture anything, so the child `target` is -1 and the reply may be any capture. Passing the quiet move's destination would only examine captures of the piece that just stepped aside.

The missing-king loss stays as it is. A dead position is still 0, and it is still recognized before the check test. [0003](0003-draw-rules.md) says a dead position cannot be mate.

### Not in check

Stand-pat stays, including the beta cutoff, for a position that has a legal move.

The tactical list is legal captures, en passant, and every promotion, including a non-capturing promotion and all four promotion pieces. `Board::isCapture` is the wrong test for that list. It is false for en passant, because the captured pawn is not on the destination square, and it is false for a non-capturing promotion.

The recapture filter still applies to captures. A non-capturing promotion is searched even when its destination is not `target`. The child of that promotion uses `target == -1`, so the stand-pat sees the new piece and the reply can capture it. A promotion that captures keeps the recapture chain, as any other capture does.

If the tactical list is empty, generate the full legal list. An empty list is stalemate and the score is 0, returned before stand-pat. A stalemate whose static score is above beta would otherwise fail high. Quiet moves found by that second generation are not searched.

`capturesOnly` stays the capture filter it is. Perft does not start generating promotions from a capture generator. The horizon selects its own subset.

### Fifty-move claim

The depth-0 block that generates moves only when the clock is at 100 goes away. Quiescence now returns the mate score, and the claim must leave it alone. A horizon score below 0 becomes 0 when the clock is at 100, except a mate loss. A mate loss is at or below `-FITNESS_LARGE / 2`. No static score reaches that range: `FITNESS_LARGE` is 1e9 and the king piece value is 1e6.

A check that still has an evasion is searched. A capture that wins material stays a win. A negative score that is not mate becomes the claim, which is path-dependent and is not stored. That is the same claim [0003](0003-draw-rules.md) applies to a horizon score. Stalemate is already 0.

### Root

`getMoveToDepth` on an empty legal list returns no move and sets the score: the mate loss if the root is in check, and 0 if it is stalemate. `getMove` keeps that score. A mated root is a loss. A stalemated root is a draw because the position is terminal, not because the score was initialized to 0.

### Tests

Add cases to `test/engine.py`. Existing eval orderings and perft stay as they are. Depth-6 tactical moves may change; update an expected move only when the new horizon is the reason.

- After `f2f3 e7e5 g2g4`, depth 1 plays `d8h4` and the score is above 100000.
- `7k/8/6K1/8/8/8/8/R7 w - - 0 1` at depth 1 plays `a1a8` and the score is above 100000. The existing clock-100 mate test stays.
- `4k3/P7/8/8/8/8/8/4K3 b - - 0 1` at depth 1 scores -500 or lower.
- `k7/P7/2K5/8/8/8/8/8 w - - 0 1` at depth 1 scores 0.
- The position after `d8h4` returns `bestmove 0000` and a score below -100000.
- `k7/P7/1K6/8/8/8/8/8 b - - 0 1` returns `bestmove 0000` and score 0.

The README's horizon sentence should say that the horizon also answers a check and plays a promotion.

## Consequences

- A mate that leaves the king on the board is a mate at depth 1. A stalemate is 0 at depth 1. A pawn on the seventh rank is a queen, rook, bishop, or knight in the horizon score.
- A checked side is no longer given the static score of standing still. A future network behind `fitness` is called for stand-pat only when a pass is legal.
- Ordinary captures remain recapture-only after the first capture. Giving check is not itself a reason to keep searching. The horizon answers a check; it does not hunt for one.
- Quiet leaves that have no capture and no promotion pay for a second legal-move generation, and those quiet moves are thrown away. That generation exists to tell a stalemate from a quiet position.
- The fifty-move claim still draws a lost horizon, and it still loses to mate. The extra generator at depth 0 is gone.
- Perft, static eval, and the draw rules in [0003](0003-draw-rules.md) stay. Stored mate scores are still raw floats, as [0002](0002-quality-pass.md) left them.
