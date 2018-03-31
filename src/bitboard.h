#ifndef SRC_BITBOARD_H_
#define SRC_BITBOARD_H_

#include "types.h"

#define SQ(rank, file) (rank * 8 + file)
#define FILE(square) ('a' + (square % 8))
#define RANK(square) ('1' + (square / 8))

extern Bitboard BB_SET[64];
extern Bitboard BB_CLR[64];
extern Bitboard BB_FILE[8];
extern Bitboard BB_RANK[8];
extern Bitboard BB_ROOK_START[2];

void initBitboards();
int popBit(Bitboard *bitboard);
bool inBounds(int position, int stepSize);

#endif /* SRC_BITBOARD_H_ */
