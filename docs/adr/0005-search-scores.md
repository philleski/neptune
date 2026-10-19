# 0005. Search scores and transposition bounds

Date: 2026-10-19

## Status

Accepted

## Context

[0002](0002-quality-pass.md) left the transposition table alone. Scores are raw floats, and correcting them changes which move comes back. [0003](0003-draw-rules.md) left mate scores on that table unadjusted. A later network will write into the same table. It has to be speaking centipawns before that happens.

The search is fail-hard. A cutoff stores `beta`, the bar for that call, and an all-node stores the `alpha` it was called with. The child score that proved the bound is discarded. On a hit, an exact score is returned, and a cutoff score can raise `alpha`. An all-node score is never read. A score is used only when the stored depth equals the depth still to search. The stored move is still tried first at any depth.

`put` always overwrites the slot, the low 22 bits of the hash. A depth-1 store replaces a depth-8 result that hashed there.

Checkmate is `-FITNESS_LARGE + ply * FITNESS_MOVE`. `FITNESS_LARGE` is 1e9 and `FITNESS_MOVE` is 10000. `ply` is the fullmove index from [0003](0003-draw-rules.md), so the number says how long the game has been, not how far the mate is from this position. The hash does not include `ply`. A later visit returns that float unchanged. A mate stored from ply 10 as `1e9 - 15 * 10000` outranks a mate in one searched fresh at ply 40, which scores `1e9 - 41 * 10000`.

`info` prints that float as `score cp`. The fifty-move mate test locks it with a centipawn floor of 100000.

## Decision

Score the search in integer centipawns, and store the bound the search actually proved.

`Evaluation::fitness` stays the hand-tuned float. Weights stay put. The search rounds a leaf to the nearest centipawn and clamps it to the closed range ±(`MATE - MAX_DEPTH`), with `MATE` at 30000 and `MAX_DEPTH` at 64, so a leaf lands in ±29936. Mate scores use the range at and beyond ±29937. `_eval` and the other debug prints stay floats.

A mate score is relative to the node that owns it. The side to move is checkmated at `-MATE`. Backing a mate score up steps it one ply toward zero: the mover who delivered mate sees `MATE - 1`, and the side mated by that line sees `-MATE + 2`. `Board::ply` is not part of the score, so the same position stores the same mate distance at every fullmove number. A shorter mate still scores higher, because `MATE - 1` beats `MATE - 5`. A missing king is the same loss as checkmate, `-MATE` at that node. Search stops reading `FITNESS_LARGE` and `FITNESS_MOVE`.

The root window is ±(`MATE + 1`). A proven mate sits inside it and is stored as an exact score.

The search is fail-soft.

- A cutoff stores the child score and the move that crossed `beta`, and returns that score.
- An all-node stores the best score it proved and the move that produced it, and returns that score.
- An exact node stores the best score inside the window.

The table no longer stores the window edges.

A probe uses an entry when the hash matches and the stored depth is at least the depth still to search.

- An exact score is returned.
- A lower bound at least `beta` is returned. A lower bound above `alpha` raises `alpha`.
- An upper bound at most `alpha` is returned. An upper bound below `beta` lowers `beta`.

Ordering may use the stored move at any depth, from any node type. The printed principal variation follows exact entries only, so an all-node move cannot extend the line.

Checkmate and stalemate are stored as exact scores, as a dead position already is. Repetition and a fifty-move claim stay path-dependent and unstored. The table is still not probed for a score when the halfmove clock is already at 100. Quiescence uses the same integer scores and returns the score it proved, including `-MATE` for a missing king. It still does not read or write the table.

Replacement keeps the deeper entry. `put` writes the slot when the slot is empty or the new depth is greater than or equal to the stored depth. A shallower store leaves the deeper entry in place. The table stays 2^22 entries and is still cleared on `ucinewgame`. The score field becomes an `int`.

UCI prints `score mate N` when the root score is a mate, and `score cp` otherwise. `N` is in moves of the mating side, and negative when this side is getting mated. From a winning mate score, `N` is `(MATE - score + 1) / 2`. From a losing mate score, `N` is `-(MATE + score) / 2`. Already checkmated is `mate 0`. Mate in one is `mate 1`.

### Tests

Mate in one stays `d8h4`, the fastest mate stays `b2b8`, and mate on a full fifty-move clock stays `a1a8`. Each of those reports `score mate` with the same `N` from a FEN at fullmove 1 and from the same board with a fullmove number far ahead. The info-line check accepts `score mate` or `score cp`. The fifty-move mate test drops the centipawn floor of 100000.

Queen versus king and the draw scores stay centipawns. `_eval` orderings stay on the float eval. The depth-6 fork is re-recorded if the new bounds change the move. Perft is unchanged.

### Out of scope

- The network, clocks, books, and tablebases. A network later returns centipawns through the same leaf rounding and clamp.
- Retuning `fitness` weights.
- Quiescence consulting the table.
- Aging. A deep entry can sit in its slot until `ucinewgame`, or until a store of greater or equal depth hits it.

## Consequences

- The move at a fixed depth can change. Mate distance follows the node, not the fullmove number.
- A GUI sees `score mate N` for a mate, instead of a centipawn near 1e9.
- A result is reused when its depth covers the search still to do. A depth-1 store no longer erases a deeper one in the same slot.
- All-node bounds prune.
- This closes the deferral in [0002](0002-quality-pass.md) and the note in [0003](0003-draw-rules.md) that mate scores in the table are not adjusted by ply. `ply` stays the fullmove index.
