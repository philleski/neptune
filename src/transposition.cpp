#include "transposition.h"

#include <cstring>

TranspositionTable::TranspositionTable() : data(SIZE) {
	clear();
}

void TranspositionTable::clear() {
	std::memset(data.data(), 0, data.size() * sizeof(TranspositionEntry));
}

void TranspositionTable::put(U64 hash, int depth, int score, Move move, int type) {
	if(depth < 0) {
		depth = 0;
	}
	if(depth > 255) {
		depth = 255;
	}
	TranspositionEntry *entry = &data[hash & (SIZE - 1)];
	// Keep a deeper result. An equal depth is a newer search of this slot.
	if(entry->type != TT_NONE && entry->depth > depth) {
		return;
	}
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
