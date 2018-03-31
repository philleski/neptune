#ifndef SRC_SLIDINGATTACK_H_
#define SRC_SLIDINGATTACK_H_

#include "bitboard.h"

struct MagicBitboard {
	Bitboard *ptr;
	Bitboard mask;
	Bitboard magic;
	int shift;
};

class SlidingAttack {
public:
	SlidingAttack();
	Bitboard getAttackSquares(int square, PieceType pieceType,
		Bitboard allPieces, Bitboard ourPieces);
private:
	void generateMagicBitboard(int square, PieceType pieceType,
		Bitboard *ptr);
	MagicBitboard magicBitboardsBishop[64];
	MagicBitboard magicBitboardsRook[64];
	// If you sum (1ULL << maxAttackSquares) over all bishop and rook squares you'll get 107648.
	Bitboard attackTable[107648];
};

#endif /* SRC_SLIDINGATTACK_H_ */
