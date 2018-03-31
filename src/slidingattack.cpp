#include <stdio.h>
#include <stdlib.h>

#include "slidingattack.h"

SlidingAttack::SlidingAttack() {
	Bitboard *ptr = attackTable;
	for(int square = A1; square <= H8; square++) {
		generateMagicBitboard(square, BISHOP, ptr);
		ptr += 1ULL << (64 - magicBitboardsBishop[square].shift);

		generateMagicBitboard(square, ROOK, ptr);
		ptr += 1ULL << (64 - magicBitboardsRook[square].shift);
	}
}

Bitboard SlidingAttack::getAttackSquares(int square, PieceType pieceType,
		Bitboard allPieces, Bitboard ourPieces) {
	MagicBitboard magicBitboard;
	if(pieceType == BISHOP) {
		magicBitboard = magicBitboardsBishop[square];
	} else if(pieceType == ROOK) {
		magicBitboard = magicBitboardsRook[square];
	}
	int offset = (int)(((allPieces & magicBitboard.mask) * magicBitboard.magic)
			>> magicBitboard.shift);
	Bitboard attackSquares = *(magicBitboard.ptr + offset);
	return (attackSquares & ~ourPieces);
}

// Modified version of this code:
// https://stackoverflow.com/questions/28115724/getting-big-random-numbers-in-c-c
U64 randomU64() {
    U64 r = 0;
    for(int i = 0; i < 5; ++i) {
        r = (r << 15) | (rand() & 0x7FFF);
    }
    return r & 0xFFFFFFFFFFFFFFFFULL;
}

void SlidingAttack::generateMagicBitboard(int square, PieceType pieceType,
		Bitboard *ptr) {
	Bitboard mask = BB_EMPTY;
	int maxAttackSquares = 0;
	srand(0);

	Direction bishopDirections[] = {NORTHWEST, NORTHEAST, SOUTHWEST, SOUTHEAST};
	Direction rookDirections[] = {NORTH, SOUTH, WEST, EAST};
	Direction *directions;
	if(pieceType == BISHOP) {
		directions = bishopDirections;
	} else if(pieceType == ROOK) {
		directions = rookDirections;
	} else {
		return;
	}

	// Don't include edges in the attack squares since they always block the
	// piece from sliding further. The exception is that if a rook starts off
	// on an edge it can continue moving along that edge.
	int s;
	for(int directionIndex = 0; directionIndex < 4; directionIndex++) {
		Direction direction = directions[directionIndex];
		if(!inBounds(square, direction)) {
			continue;
		}
		s = square + direction;
		while(inBounds(s, direction)) {
			mask |= BB_SET[s];
			maxAttackSquares++;
			s += direction;
		}
	}
	int shift = 64 - maxAttackSquares;

	int maskOneBitLocations[maxAttackSquares];
	Bitboard maskTemp = mask;
	for(int i = 0; i < maxAttackSquares; i++) {
		maskOneBitLocations[i] = popBit(&maskTemp);
	}

	int numOccupancies = 1ULL << maxAttackSquares;
	Bitboard occupancies[numOccupancies];
	Bitboard attackSquaresByOccupancy[numOccupancies];
	for(int occupancyIndex = 0; occupancyIndex < numOccupancies; occupancyIndex++) {
		occupancies[occupancyIndex] = BB_EMPTY;
		attackSquaresByOccupancy[occupancyIndex] = BB_EMPTY;
		for(int i = 0; i < maxAttackSquares; i++) {
			bool isBitBlocker = ((occupancyIndex & (1 << i)) != 0);
			if(isBitBlocker) {
				occupancies[occupancyIndex] |= BB_SET[maskOneBitLocations[i]];
			}
		}
		for(int directionIndex = 0; directionIndex < 4; directionIndex++) {
			Direction direction = directions[directionIndex];
			if(!inBounds(square, direction)) {
				continue;
			}
			s = square + direction;
			while(1) {
				attackSquaresByOccupancy[occupancyIndex] |= BB_SET[s];
				// Count the blocker as an attack square since it may be an
				// enemy piece. Don't keep going in the same direction
				// looking for more attack squares though.
				if(BB_SET[s] & occupancies[occupancyIndex]) {
					break;
				}
				if(!inBounds(s, direction)) {
					break;
				}
				s += direction;
			}
		}
	}

	Bitboard magic = BB_EMPTY;
	Bitboard attackSquaresByOffset[numOccupancies];
	while(1) {
		// Generate a sparse random number. This will on average have
		// 64 * 1/2 * 1/2 * 1/2 = 8 bits.
		magic = randomU64() & randomU64() & randomU64();

		for(int i = 0; i < numOccupancies; i++) {
			attackSquaresByOffset[i] = BB_EMPTY;
		}

		bool duplicate = false;
		for(int occupancyIndex = 0; occupancyIndex < numOccupancies;
				occupancyIndex++) {
			int offset = (int)((occupancies[occupancyIndex] * magic) >> shift);
			if(attackSquaresByOffset[offset] && attackSquaresByOffset[offset] !=
					attackSquaresByOccupancy[occupancyIndex]) {
				duplicate = true;
				break;
			}
			attackSquaresByOffset[offset] = attackSquaresByOccupancy[occupancyIndex];
		}
		if(!duplicate) {
			break;
		}
	}

	for(int offset = 0; offset < numOccupancies; offset++) {
		*(ptr + offset) = attackSquaresByOffset[offset];
	}

	MagicBitboard magicBitboard;
	magicBitboard.ptr = ptr;
	magicBitboard.mask = mask;
	magicBitboard.magic = magic;
	magicBitboard.shift = shift;

	if(pieceType == BISHOP) {
		magicBitboardsBishop[square] = magicBitboard;
	} else if(pieceType == ROOK) {
		magicBitboardsRook[square] = magicBitboard;
	}
}
