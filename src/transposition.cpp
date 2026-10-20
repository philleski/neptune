#include "transposition.h"

#include <cstring>
#include <new>

// 64 MiB. An entry is 16 bytes, so this is 4,194,304 slots.
static const int DEFAULT_ENTRIES = 1 << 22;

// The index is a mask, so the count stays a power of two that fits in the request.
static int entriesForMegabytes(int megabytes) {
	long long bytes = (long long) megabytes * 1024 * 1024;
	long long slots = bytes / (long long) sizeof(TranspositionEntry);
	long long count = 1;
	while(count * 2 <= slots) {
		count *= 2;
	}
	return (int) count;
}

TranspositionTable::TranspositionTable() : data(DEFAULT_ENTRIES) {
	clear();
}

void TranspositionTable::clear() {
	std::memset(data.data(), 0, data.size() * sizeof(TranspositionEntry));
}

void TranspositionTable::resize(int megabytes) {
	int count = entriesForMegabytes(megabytes);
	if(count == (int) data.size()) {
		return;
	}
	// Allocate first. A failure leaves the table the search already has.
	try {
		std::vector<TranspositionEntry> next((size_t) count);
		data.swap(next);
	} catch(const std::bad_alloc &) {
		return;
	}
	clear();
}

int TranspositionTable::entries() const {
	return (int) data.size();
}

void TranspositionTable::put(U64 hash, int depth, int score, Move move, int type) {
	if(depth < 0) {
		depth = 0;
	}
	if(depth > 255) {
		depth = 255;
	}
	TranspositionEntry *entry = &data[hash & (data.size() - 1)];
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
	const TranspositionEntry *found = &data[hash & (data.size() - 1)];
	if(found->type == TT_NONE || found->hash != hash) {
		return false;
	}
	*entry = *found;
	return true;
}
