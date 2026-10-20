#ifndef SRC_TRANSPOSITION_H_
#define SRC_TRANSPOSITION_H_

#include <vector>

#include "types.h"

enum TranspositionType {
	TT_NONE = 0, TT_PV = 1, TT_CUT = 2, TT_ALL = 3
};

struct TranspositionEntry {
	U64 hash;
	int score;
	unsigned short move;
	unsigned char depth;
	unsigned char type;
};

class TranspositionTable {
public:
	TranspositionTable();
	void clear();
	// Megabytes are 1024 * 1024 bytes. The same entry count leaves the table alone.
	void resize(int megabytes);
	int entries() const;
	void put(U64 hash, int depth, int score, Move move, int type);
	bool get(U64 hash, TranspositionEntry *entry) const;
private:
	std::vector<TranspositionEntry> data;
};

#endif /* SRC_TRANSPOSITION_H_ */
