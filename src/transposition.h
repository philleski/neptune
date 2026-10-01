#ifndef SRC_TRANSPOSITION_H_
#define SRC_TRANSPOSITION_H_

#include <vector>

#include "types.h"

enum TranspositionType {
	TT_NONE = 0, TT_PV = 1, TT_CUT = 2, TT_ALL = 3
};

struct TranspositionEntry {
	U64 hash;
	float score;
	unsigned short move;
	unsigned char depth;
	unsigned char type;
};

class TranspositionTable {
public:
	TranspositionTable();
	void clear();
	void put(U64 hash, int depth, float score, Move move, int type);
	bool get(U64 hash, TranspositionEntry *entry) const;
private:
	static const int SIZE_BITS = 22;
	static const int SIZE = 1 << SIZE_BITS;
	std::vector<TranspositionEntry> data;
};

#endif /* SRC_TRANSPOSITION_H_ */
