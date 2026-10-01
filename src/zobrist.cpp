#include "zobrist.h"

#include <random>

static U64 pieceKeys[12][64];
static U64 sideKey;
static U64 epKeys[8];
static U64 castleKeys[16];
static bool ready = false;

static U64 nextKey(std::mt19937_64 *rng) {
	U64 key = 0;
	while(key == 0) {
		key = (*rng)();
	}
	return key;
}

void initZobrist() {
	if(ready) {
		return;
	}
	std::mt19937_64 rng(0);
	for(int piece = 0; piece < 12; piece++) {
		for(int square = 0; square < 64; square++) {
			pieceKeys[piece][square] = nextKey(&rng);
		}
	}
	sideKey = nextKey(&rng);
	for(int file = 0; file < 8; file++) {
		epKeys[file] = nextKey(&rng);
	}
	for(int rights = 0; rights < 16; rights++) {
		castleKeys[rights] = nextKey(&rng);
	}
	ready = true;
}

U64 zobristPiece(int piece, int square) {
	return pieceKeys[piece][square];
}

U64 zobristSide() {
	return sideKey;
}

U64 zobristEp(int file) {
	return epKeys[file];
}

U64 zobristCastle(unsigned int castleRights) {
	return castleKeys[castleRights & 15];
}
