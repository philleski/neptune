#include "bitboard.h"

Bitboard BB_SET[64];
Bitboard BB_CLR[64];
Bitboard BB_FILE[8];
Bitboard BB_RANK[8];
Bitboard BB_ROOK_START[2];

void initBitboards() {
	for(int i = 0; i < 8; i++) {
		BB_FILE[i] = BB_EMPTY;
		BB_RANK[i] = BB_EMPTY;
	}
	for(int square = A1; square <= H8; square++) {
		BB_SET[square] = 1ULL << square;
		BB_CLR[square] = ~BB_SET[square];
		BB_FILE[square % 8] |= BB_SET[square];
		BB_RANK[square / 8] |= BB_SET[square];
	}
	BB_ROOK_START[WHITE] = BB_SET[A1] | BB_SET[H1];
	BB_ROOK_START[BLACK] = BB_SET[A8] | BB_SET[H8];
}

// Implementation from here: https://www.youtube.com/watch?v=ITVB7JSaI3w
const int BitTable[64] = {
	63, 30, 3, 32, 25, 41, 22, 33, 15, 50, 42, 13, 11, 53, 19, 34, 61, 29, 2,
	51, 21, 43, 45, 10, 18, 47, 1, 54, 9, 57, 0, 35, 62, 31, 40, 4, 49, 5, 52,
	26, 60, 6, 23, 44, 46, 27, 56, 16, 7, 39, 48, 24, 59, 14, 12, 55, 38, 28,
	58, 20, 37, 17, 36, 8
};

int popBit(Bitboard *bitboard) {
	Bitboard b = *bitboard ^ (*bitboard - 1);
	unsigned int fold = (unsigned)((b & 0xffffffff) ^ (b >> 32));
	*bitboard &= (*bitboard - 1);
	return BitTable[(fold * 0x783a9b23) >> 26];
}

bool inBounds(int square, int stepSize) {
	if(square + stepSize < 0) {
		return false;
	}
	if(square + stepSize >= 64) {
		return false;
	}
	int fileDiff = (square % 8) - ((square + stepSize) % 8);
	if(fileDiff < -2) {
		return false;
	}
	if(fileDiff > 2) {
		return false;
	}
	return true;
}
