#ifndef SRC_ZOBRIST_H_
#define SRC_ZOBRIST_H_

#include "types.h"

void initZobrist();
U64 zobristPiece(int piece, int square);
U64 zobristSide();
U64 zobristEp(int file);
U64 zobristCastle(unsigned int castleRights);

#endif /* SRC_ZOBRIST_H_ */
