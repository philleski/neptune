#include "transposition.h"

#include <cstring>

TranspositionTable::TranspositionTable() : data(SIZE) {
	clear();
}

void TranspositionTable::clear() {
	std::memset(data.data(), 0, data.size() * sizeof(TranspositionEntry));
}

void TranspositionTable::put(U64 hash, int depth, float score, Move move, int type) {
	TranspositionEntry *entry = &data[hash & (SIZE - 1)];
	entry->hash = hash;
	entry->score = score;
	entry->move = (unsigned short) move;
	entry->depth = (unsigned char) depth;
	entry->type = (unsigned char) type;
}

bool TranspositionTable::get(U64 hash, TranspositionEntry *entry) const {
	const TranspositionEntry *found = &data[hash & (SIZE - 1)];
	if(found->type == TT_NONE || found->hash != hash) {
		return false;
	}
	*entry = *found;
	return true;
}
