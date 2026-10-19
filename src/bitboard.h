#ifndef SRC_BITBOARD_H_
#define SRC_BITBOARD_H_

#include "types.h"

#define SQ(rank, file) ((rank) * 8 + (file))

extern Bitboard BB_SET[64];
extern Bitboard BB_CLR[64];
extern Bitboard BB_FILE[8];
extern Bitboard BB_RANK[8];
extern Bitboard BB_ROOK_START[2];

void initBitboards();
int popBit(Bitboard *bitboard);
// The builtin call stays in this function, so a compiler without it is handled here.
inline int countBits(Bitboard bitboard) {
#if defined(__clang__) || defined(__GNUC__)
	return __builtin_popcountll(bitboard);
#else
#error countBits has no fallback for this compiler
#endif
}
bool inBounds(int position, int stepSize);

#endif /* SRC_BITBOARD_H_ */
