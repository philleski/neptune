# 0003. Draw rules in the search

Date: 2026-10-19

## Status

Accepted

## Context

Search ends a game only for checkmate and stalemate. Checkmate is a loss. Stalemate returns 0. A missing king is still a loss.

The FEN halfmove clock is read in `Board::setPosition` and then discarded. `Board::ply` is `2 * (fullmove - 1)` plus one when black is to move. That value indexes `undoHistory` and scales the mate penalty (`FITNESS_LARGE` and `FITNESS_MOVE`). It is not a fifty-move counter.

`Undo` already stores `positionHash` for every `Board::move`, including the moves UCI applied. The Zobrist key covers placement, side to move, castling rights, and the en passant file, which is enough to identify a repeated position. It does not cover the halfmove clock, so a fifty-move draw depends on the path. A repetition does too.

Nothing walks those hashes. Nothing tests material that cannot mate. A line that repeats is searched and can be stored as a win. A lost position will not take a perpetual, because the perpetual is scored like any other move.

Two limits sit on the history. `setPosition` jumps `ply` to the fullmove index, so undo entries below that ply are empty. A FEN does not carry the positions that produced its clock. Only moves played after `setPosition` are on the stack, which is the whole game for `position startpos moves`. And checkmate is still a loss when the clock is already at 100. A dead position cannot be mate.

## Decision

Keep `ply` as the fullmove index so mate scores stay the same. Store the halfmove clock on the board and on `Undo`.

`setPosition` keeps the parsed clock, or 0 when the field is absent, and records `historyPly`, the ply where undo hashes start. `move` resets the clock on a pawn move, a capture, or en passant, and increments it otherwise. En passant is a pawn capture even though the captured pawn is not on the destination square. `isCapture` only tests that square.

A position is a draw in three cases, checked at the root and at each `alphabeta` node.

- Threefold repetition. The current key occurred at least twice before, on the same side, in `undoHistory` from `historyPly` through the last `halfmoveClock` plies. One earlier copy is not a draw. This is checked before the transposition table and is not stored. The score is 0.
- Dead position. Both kings are on the board and mate is impossible: king versus king, king and knight versus king, king and bishop versus king, or king and bishop versus king and bishop on the same color. King and two knights versus king is still searched. This score is 0 and may be stored, because every path agrees. Static evaluation is unchanged.
- Fifty-move rule. When `halfmoveClock` is at least 100, the side to move can claim a draw, so a negative score becomes 0. That 0 is path-dependent and is not stored, nor is a parent bound that came from it. A checkmate already on the board stays a loss. A move that checkmates still wins, and so does any line better than a draw, including a capture or pawn move that resets the clock. Quiescence does not see repetitions. It does treat a dead position as 0. The fifty-move claim is applied to a horizon score before it is returned.

The table is not probed for a score when the clock is already at 100, so an older win for the same key cannot hide the claim. The table move may still order moves.

At the root, a repetition or a dead position returns a legal move and score 0 without searching. A fifty-move root is searched, so mate in one is still played.

## Consequences

- Lines that repeat or run out the fifty-move clock score as draws, and a lost position can take the draw.
- Mate distance for positions that are not draws is unchanged, because `ply` is unchanged.
- Perft and static eval are unchanged. A bare king endgame still has a king-activity score from `_eval`. Search scores that position as 0.
- A FEN with a non-zero clock does not reconstruct earlier positions. Repetition from those positions is invisible until the GUI sends the moves.
- Transposition hits for ordinary positions stay as they are. Mate scores in the table are still not adjusted by ply.
