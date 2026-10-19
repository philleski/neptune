# 0006. Time control

Date: 2026-10-19

## Status

Accepted

## Context

A GUI cannot play a clocked game. [0001](0001-port-tactician-engine.md) left time controls unported, and [0002](0002-quality-pass.md) and [0005](0005-search-scores.md) left them out of scope. The hole is still there.

`go` in `main` sets depth to 8, then reads tokens only to notice `depth`. `wtime`, `btime`, `winc`, `binc`, `movestogo`, and `movetime` are skipped. `infinite` is skipped with them. There is no `stop` command. The loop then calls `Brain::getMove` and does not read stdin again until that call returns, so a `stop` that arrived during the search would sit unread even if the command existed.

`getMove` walks depths from 1 to the requested depth and returns when the last one finishes. Nothing counts nodes. Nothing reads the clock. `elapsedMillis` exists for `_perft` and is not used by the search. The only `info` line is printed after the whole search, with the final depth, score, and principal variation.

Iterative deepening keeps the move of a finished depth in `SearchResult`, then replaces it when the next depth returns. It throws away the information the next depth would need in order to be stopped safely.

`getMoveToDepth` starts with `unsetKillers` and generates the root with `MOVE_NONE`. The root does not probe the table. `orderMoves` still lifts captures in front of quiet moves, and it still has a slot for a table move and a killer, but both are empty: the table move was passed as `MOVE_NONE`, and the killers were just cleared. The first move searched is the first generated capture, or the first generated quiet move when the position has no capture. Child nodes do use the table. Entries written below the root survive into the next depth. The root itself is not one of them. The comment in `getMoveToDepth` stands: the root window is not a table entry.

An iteration that returned halfway through the root would hand back that first searched move. `bestMove` starts as `MOVE_NONE` and takes `moves[0]` as soon as the first root move returns. `getMove` would store that as the result and drop the move the previous depth had finished. There is no abort today. Adding a deadline or `stop` on top of this loop would commit that partial root.

`go depth N` is what `test/engine.py` sends. Bare `go` is depth 8, which is what the README describes. Those stay the fixed-depth interface. A clock, `movetime`, `infinite`, and `stop` are the missing ones.

## Decision

Search until a finished iteration says the budget is spent, a requested depth is done, or `stop` arrives. The move that comes back is always from a finished iteration. The next iteration tries that move first.

### Limits

`go` reads `depth`, `infinite`, `movetime`, `wtime`, `btime`, `winc`, `binc`, and `movestogo`. Times are milliseconds. A keyword takes the following integer. `infinite` does not. An unrecognized token is skipped, including `ponder`, `nodes`, `mate`, and `searchmoves`.

- `depth N` with no clock and no `movetime` searches N plies and has no deadline. Bare `go` is `depth 8`.
- `infinite` searches until `stop`, up to `MAX_DEPTH - 1`.
- `movetime T` sets the soft limit and the hard limit to T. Clock tokens on the same `go` are ignored.
- Otherwise, if the side to move has a clock (`wtime` when White is to move, `btime` when Black is), that clock is the move's time. The other side's clock is not used. A missing increment is 0. `movestogo` when it is present and positive is the move count; otherwise the count is 20.
- `depth` together with a clock or `movetime` stops at whichever limit arrives first.

The budget, in milliseconds:

- `usable` is `remaining - 50` when more than 50 ms are left, and `remaining` otherwise. The 50 ms is time to print `bestmove` before the flag.
- `soft` is `remaining / moves + increment`, capped at `usable`.
- `hard` is `soft * 2`, capped at `usable`.

`movetime` skips that formula and sets both limits to T. A remaining time of 0 leaves both limits at 0.

Depth 1 always runs to completion, including when the limits are already 0. After a finished depth, the next depth starts only when three things are true: the requested depth is not done, `stop` has not been seen, and the clock still has room. The clock still has room when the soft limit is not yet reached and the depth just finished took less time than what remains of the soft limit. The next depth is expected to take at least as long as this one, so starting it would miss the soft limit.

The hard limit is polled during depths after the first. Crossing it abandons the depth in progress. That depth's root move is discarded. The last finished depth is the one that is returned.

### Stop

The search stays on the thread that owns the board. There is no second thread. Every 2048 nodes, and before each depth including the first, the search calls back into the UCI loop, which polls stdin and handles every complete line already buffered. A `stop` that is already waiting is handled before depth 1 spends its nodes.

- `stop` abandons the depth in progress, including depth 1.
- `isready` prints `readyok` and the search continues.
- `quit` during `go infinite` abandons the search, prints no `bestmove`, and exits. Behind a depth or a clock, `quit` stays buffered until `bestmove` is printed, and the process exits after that. A piped script ends with `quit`, and that command belongs to the loop after the search.
- Any other line is ignored.

A node is one call of `alphabeta` or `quiescentSearch`. The counter starts at 0 for each `go`.

If `stop` or the hard limit fires inside a node, that node stores nothing and returns an abort to its parent. Children that already finished have already stored their own entries, and those entries stay. The parent does not treat the aborted child as a result.

If `stop` arrives during depth 1 before that depth finishes, there is no finished iteration yet. The move is the best root move that has already returned. If the first root move has not returned, the move is the first move in the root list, which is a legal move. A hard-limit expiry does not take this path: depth 1 runs to the end.

### Root order

`unsetKillers` runs once at the start of `getMove`, and again from `clear` and the constructor, as it does now. It does not run at the start of each depth. Killers recorded while searching depth D are still there for depth D + 1.

The root ordering move is the best move of the depth just finished. On depth 1 that move does not exist yet, so the root probes the table and uses the stored move when there is one. The probe is for ordering only. A stored score does not cut the root off, and a finished root is still not written back. The root window stays off the table, as [0005](0005-search-scores.md) left it. `orderMoves` is unchanged: that move, then captures, then killers, then quiet moves.

`getMove` keeps the result of the last finished depth and replaces it only when the next depth finishes. An abandoned depth leaves the stored result in place.

A repetition or a dead position at the root still returns the first legal move and does not search, as it does now.

### Output

Each finished depth prints an `info` line before the next depth starts: depth, `score cp` or `score mate`, `nodes`, `time` in milliseconds, and `pv`. `bestmove` is printed once, after the search returns. `go depth N` still ends on the info line for that depth, or for the depth that hit mate or stalemate at the root.

### Tests

`test/engine.py` reads every `info` line before `bestmove` and scores the last one. Existing `go depth` cases keep their moves and their scores. Re-record a case only when the new root order changes the move. Mate in one, the fastest mate, and mate on a full fifty-move clock are the cases expected to stay put.

The driver already writes the whole script before the engine reads it, so a following line is buffered during the search.

- `go movetime 0` on the mate-in-one position plays `d8h4`, the depth-1 move, and the last info depth is 1.
- `go wtime 0 btime 0` from the start position returns a legal move and the last info depth is 1.
- `go infinite`, then `isready`, then `stop`, prints `readyok`, then an info line, then `bestmove`, and the process exits. The test uses a short timeout, so a search that never reads `stop` fails.
- `go depth 6` with no clock still searches depth 6.

Perft, `_eval`, and the draw rules are unchanged.

### README

The opening description says the search looks a fixed number of plies ahead. It should say that `go depth N` does that, and that a clocked `go` searches until the budget above runs out. The command list includes `stop`. Bare `go` remains 8 plies.

### Out of scope

- Pondering, `ponderhit`, `go nodes`, `go mate`, and `searchmoves`.
- A second search thread, aspiration windows, and a node-count or mate-distance limit.
- Books, tablebases, and the network.
- Retuning the 20-move assumption, the 50 ms reserve, or the hard limit of twice the soft limit. Those are the first numbers, not a claim about the best allocation.
- Storing the root in the transposition table.

## Consequences

- A GUI can send a clock, `movetime`, or `go infinite` and can interrupt the search with `stop`.
- The returned move was searched to a finished depth. An abandoned deeper iteration does not replace it.
- Depth 1 can run past a short clock. Later depths cannot run past the hard limit, and they do not start once the soft limit is already gone.
- `go depth N` can change which move comes back, because the root tries the previous best move first and killers survive from one depth to the next. The score of a full-window search of every root move is the same search; the table sees those moves in a different order.
- This closes the clock deferral in [0001](0001-port-tactician-engine.md), [0002](0002-quality-pass.md), and [0005](0005-search-scores.md). Books, tablebases, and the network stay deferred.
