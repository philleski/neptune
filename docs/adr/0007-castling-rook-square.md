# 0007. Castling rights on rook moves

Date: 2026-10-20

## Status

Accepted

## Context

A castling right belongs to one rook, the one that started on a1, h1, a8, or h8. Moving that rook, or capturing it, drops the right. Moving a different rook does not.

`Board::updateCastleRightsRookMove` drops the right for the whole file. A rook whose source square is anywhere on the a-file clears queenside for the side to move. Anywhere on the h-file clears kingside. The destination is not consulted. The corner rook can still be home.

```322:328:src/board.cpp
void Board::updateCastleRightsRookMove(Bitboard occSource) {
	if(BB_FILE[FILE_A] & occSource) {
		clearCastleRight((CastleRight) CASTLE_RIGHT(this->turn, QUEENSIDE));
	} else if(BB_FILE[FILE_H] & occSource) {
		clearCastleRight((CastleRight) CASTLE_RIGHT(this->turn, KINGSIDE));
	}
}
```

A king move still clears both rights through `removeCastleRights`. That path is the right one for the king. `unmove` copies `castleRights` back out of `Undo` and does not recompute them.

Captures are already limited to the corner. `move` calls `updateCastleRightsRookCapture` only when the captured piece is a rook and the destination is in `BB_ROOK_START` for the opponent, which is a1 and h1 for White and a8 and h8 for Black. The helper then tests the file. Each of those four squares sits on the matching file, so a capture clears the right that belongs to that square. Capturing a rook on a4 leaves the opponent's queenside right in place.

Measured on the current binary, by comparing `_hash` after a move with the hash of the FEN that keeps the right and the FEN that drops it. The hash includes the rights and ignores the clocks.

- `4k3/8/8/8/R7/8/8/R3K3 w Q - 0 1` then `a4b4`. The rook on a4 steps off the a-file. The rook on a1 is unmoved. The hash matches the position with no rights, not the position that still has `Q`.
- The same position, then `a4a5`. The rook stays on the a-file. The hash still matches the position with no rights.
- `4k3/8/8/8/7R/8/8/4K2R w K - 0 1` then `h4g4`. The h1 rook is unmoved. Kingside is gone.
- `r3k3/8/8/8/8/8/r7/4K3 b q - 0 1` then `a2b2`. Black queenside is gone, and the rook on a8 is unmoved.
- `4k3/P7/8/8/8/8/8/R3K3 w Q - 0 1` then `a7a8r e8e7 a8b8`. The move that promotes is a pawn move, so it does not touch the rights. The new rook then steps off the a-file, and queenside is gone while the rook on a1 is still home.
- `r3k3/8/8/8/r7/8/8/R3K3 w Qq - 0 1` then `a1a4`. White's queenside goes, because the a1 rook moved. Black's queenside stays, because the captured rook was on a4.
- `r3k3/8/8/8/8/8/8/R3K3 w Qq - 0 1` then `a1a8`. The captured rook is on a8, and Black's queenside goes.

Perft counts leaves, so a wrong right shows up only when a later ply would have been allowed to castle. The published suite never does. From the start position a second rook exists only after a promotion, and depth 6 ends before that rook can move and the king can castle afterwards. The other published positions either have no spare rook on the castling file or have no right left to lose.

An independent count (python-chess 1.11.2) against this binary:

| Position | Depth | Legal leaves | This binary |
|---|---:|---:|---:|
| `4k3/8/8/8/8/8/R7/R3K3 w Q - 0 1` | 1 | 22 | 22 |
| same | 2 | 96 | 96 |
| same | 3 | 2545 | 2490 |
| `4k3/8/8/8/7R/8/8/4K2R w K - 0 1` | 3 | 2695 | 2640 |
| `4k3/P7/8/8/8/8/8/R3K3 w Q - 0 1` | 4 | 10184 | 10184 |
| same | 5 | 238690 | 238462 |

The first position has White rooks on a1 and a2, the king on e1, and queenside rights. Depths 1 and 2 match, because the bad clear happens on White's first move and White castles on the third ply. The 55 missing leaves are the 55 castling moves that are still legal on that ply. The kingside twin, a rook on h4 and the corner rook on h1, is short by the same 55. The promotion position matches through depth 4 and splits at depth 5, which is the first ply where the promoted rook has left the a-file and the king can castle.

## Decision

Clear a rook's castling right only when that rook moves from its corner square, or is captured there.

### Rook moves

`updateCastleRightsRookMove` tests the source square against the corner for the side to move:

- White queenside is a1. White kingside is h1.
- Black queenside is a8. Black kingside is h8.

`CASTLE_ROOK_START` already names those squares. A source on a2 through a8, or on h2 through h8, leaves White's rights alone. The same for Black on the eighth-rank corners and the rest of the file.

The corner rook still loses the right on every move, including a move that stays on the file. a1-a4 drops queenside. a4-a5 and a4-b4 do not, while the rook on a1 is home. A test that keys off "the rook changed file" is the wrong fix: it would keep the right after a1-a4, and it would still drop the right after a4-b4.

A promotion is a pawn move. The pawn path does not clear rights. The new rook is a rook on its next move, and that move uses the same helper.

### Captures

Keep the current results. A capture clears the opponent's right when the captured piece is a rook on a1, h1, a8, or h8. A capture on any other square leaves the rights alone.

Point `updateCastleRightsRookCapture` at those four squares, the same test as the move helper, instead of the file masks. The call stays guarded by "the captured piece is a rook". The file masks in both helpers go away. This does not change the capture counts: the call is already restricted to `BB_ROOK_START`, and each of those squares maps to the right the file test was clearing.

### Unchanged

A king move still clears both of that side's rights. Castling still moves the rook from `CASTLE_ROOK_START` to `CASTLE_ROOK_END`. `unmove` still restores the saved rights.

Generation still trusts the rights bits. It does not also ask whether a rook is standing on the corner. A FEN that grants a right with an empty corner is unchanged by this fix.

### Tests

Add one position to `test/perft.py`, and run it from both `make test` and `make test-perft`. The published positions stay the published suite.

- `4k3/8/8/8/8/8/R7/R3K3 w Q - 0 1` at depth 3 is 2545.

The current binary returns 2490 there. Depths 1 and 2 of that position already match, so the assertion is the depth-3 total. The published numbers stay, including the start-position smoke count.

One sentence in the README: the perft run is the published suite plus this position. `make test` runs the shallow start position and this one.

`test/engine.py` stays. Its positions do not move a second rook on the a-file or the h-file while the corner rook is home.

### Out of scope

- Chess960. The castling rook is not fixed to a1, h1, a8, and h8.
- Rejecting a FEN whose rights are set while the corner is empty.
- A second perft for the promotion line. Depth 5 of `4k3/P7/8/8/8/8/8/R3K3 w Q - 0 1` is 238690, and it is the same helper. The depth-3 position fails if that helper still tests the file.

## Consequences

- A rook other than the corner rook can move along the a-file or the h-file, or leave it, and the unmoved corner rook keeps its right. A promoted rook is included.
- White can castle on the ply after that move. Search will see the castle in positions that have a spare rook on the file. The positions `test/engine.py` already plays do not, so their moves and scores stay.
- The published perft totals stay. The new position moves from 2490 leaves to 2545.
- Capturing a rook still drops a right only on a1, h1, a8, or h8.
