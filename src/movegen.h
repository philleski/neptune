#ifndef SRC_MOVEGEN_H_
#define SRC_MOVEGEN_H_

#include "board.h"
#include "slidingattack.h"
#include "types.h"

const char *printMove(Move move);

class MoveGen {
public:
	MoveGen();
	Move *legalMovesFast(Board *board, Move *moves);
	bool isLegal(Board *board, Move move);
private:
	void initAttackSquaresPawn(Color color);
	void initAttackSquaresShortRange(int stepSizes[],
		Bitboard *attackSquares);

	Move *appendMovesForPawn(Board *board, Move *moves);
	Move *appendMovesForShortRangePiece(Bitboard bitboard,
		Bitboard *attackSquares, Board *board, Move *moves);
	Move *appendMovesForLongRangePiece(Bitboard bitboard,
		PieceType movementType, Board *board, Move *moves);
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
