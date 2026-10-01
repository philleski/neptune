#ifndef SRC_MOVEGEN_H_
#define SRC_MOVEGEN_H_

#include "board.h"
#include "slidingattack.h"
#include "types.h"

class MoveGen {
public:
	MoveGen();
	// Pseudo-legal moves. isLegal rejects moves that leave the king in check.
	Move *generateMoves(Board *board, Move *moves, bool capturesOnly = false);
	bool isLegal(Board *board, Move move);
	bool inCheck(Board *board);
private:
	void initAttackSquaresPawn(Color color);
	void initAttackSquaresShortRange(int stepSizes[],
		Bitboard *attackSquares);

	Move *appendMovesForPawn(Board *board, Move *moves, bool capturesOnly);
	Move *appendMovesForShortRangePiece(Bitboard bitboard,
		Bitboard *attackSquares, Board *board, Move *moves, bool capturesOnly);
	Move *appendMovesForLongRangePiece(Bitboard bitboard,
		PieceType movementType, Board *board, Move *moves, bool capturesOnly);
	Move *appendMovesForCastling(Board *board, Move *moves);

	Bitboard attackSquaresPawnMove[2][64];
	Bitboard attackSquaresPawnCapture[2][64];
	Bitboard attackSquaresKnight[64];
	Bitboard attackSquaresKing[64];

	Bitboard castleBlockers[4];
	int castlePath[4][3];

	bool isSquareAttacked(Board *board, Square square);

	SlidingAttack slidingAttack;
};

#endif /* SRC_MOVEGEN_H_ */
