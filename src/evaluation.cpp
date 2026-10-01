#include "evaluation.h"

#include "bitboard.h"

const float Evaluation::FITNESS_LARGE = 1000000000.0f;
const float Evaluation::FITNESS_MOVE = 10000.0f;

static int countBits(Bitboard bitboard) {
	return __builtin_popcountll(bitboard);
}

static int kingSquare(Bitboard kings) {
	if(kings == BB_EMPTY) {
		return -1;
	}
	return popBit(&kings);
}

static Bitboard flipRanks(Bitboard bitboard) {
	return __builtin_bswap64(bitboard);
}

Evaluation::Evaluation() : pawnTable(PAWN_TABLE_SIZE) {
	fitnessPiece[PAWN] = 100.0f;
	fitnessPiece[KNIGHT] = 320.0f;
	fitnessPiece[BISHOP] = 333.0f;
	fitnessPiece[ROOK] = 510.0f;
	fitnessPiece[QUEEN] = 880.0f;
	fitnessPiece[KING] = 1000000.0f;
	fitnessStartNoKing = 2 * fitnessPiece[ROOK] + 2 * fitnessPiece[KNIGHT]
		+ 2 * fitnessPiece[BISHOP] + fitnessPiece[QUEEN]
		+ 8 * fitnessPiece[PAWN];

	fitnessRookOpenFile = 50.0f;
	fitnessRookSemiOpenFile = 25.0f;
	fitnessCastleRightQueenside = 15.0f;
	fitnessCastleRightKingside = 30.0f;
	fitnessBishopPairBonus = 50.0f;
	fitnessKingRankFactor = 75.0f;

	float opening[8][4] = {
		{0, 0, 0, 0}, {90, 95, 105, 110}, {90, 95, 105, 115}, {90, 95, 110, 120},
		{97, 103, 117, 127}, {106, 112, 125, 140}, {117, 122, 134, 159}, {0, 0, 0, 0}
	};
	float endgame[8][4] = {
		{0, 0, 0, 0}, {120, 105, 95, 90}, {120, 105, 95, 90}, {125, 110, 100, 95},
		{133, 117, 107, 100}, {145, 129, 116, 105}, {161, 146, 127, 110}, {0, 0, 0, 0}
	};
	float kingFile[8] = {0, 0, -90, -180, -180, -90, 0, 0};
	for(int rank = 0; rank < 8; rank++) {
		for(int centrality = 0; centrality < 4; centrality++) {
			fitnessPawnTableOpening[rank][centrality] = opening[rank][centrality];
			fitnessPawnTableEndgame[rank][centrality] = endgame[rank][centrality];
		}
	}
	for(int file = 0; file < 8; file++) {
		fitnessKingFile[file] = kingFile[file];
	}

	pawnShieldQueenside[WHITE] = BB_SET[A2] | BB_SET[B2] | BB_SET[C2];
	pawnShieldQueensideForward[WHITE] = BB_SET[A3] | BB_SET[B3] | BB_SET[C3];
	pawnShieldKingside[WHITE] = BB_SET[F2] | BB_SET[G2] | BB_SET[H2];
	pawnShieldKingsideForward[WHITE] = BB_SET[F3] | BB_SET[G3] | BB_SET[H3];
	pawnShieldQueenside[BLACK] = flipRanks(pawnShieldQueenside[WHITE]);
	pawnShieldQueensideForward[BLACK] = flipRanks(pawnShieldQueensideForward[WHITE]);
	pawnShieldKingside[BLACK] = flipRanks(pawnShieldKingside[WHITE]);
	pawnShieldKingsideForward[BLACK] = flipRanks(pawnShieldKingsideForward[WHITE]);

	for(int square = 0; square < 64; square++) {
		passedPawnMasks[WHITE][square] = BB_EMPTY;
		passedPawnMasks[BLACK][square] = BB_EMPTY;
	}
	for(int square = 8; square < 56; square++) {
		for(int ahead = square; ahead < 56; ahead += 8) {
			passedPawnMasks[WHITE][square] |= BB_SET[ahead];
			if(square % 8 != 0) {
				passedPawnMasks[WHITE][square] |= BB_SET[ahead - 1];
			}
			if(square % 8 != 7) {
				passedPawnMasks[WHITE][square] |= BB_SET[ahead + 1];
			}
		}
		for(int ahead = square; ahead >= 8; ahead -= 8) {
			passedPawnMasks[BLACK][square] |= BB_SET[ahead];
			if(square % 8 != 0) {
				passedPawnMasks[BLACK][square] |= BB_SET[ahead - 1];
			}
			if(square % 8 != 7) {
				passedPawnMasks[BLACK][square] |= BB_SET[ahead + 1];
			}
		}
	}
}

float Evaluation::endgameFraction(Board *board) {
	float material = 0;
	Color opponent = FLIP(board->turn);
	for(int piece = PAWN; piece < KING; piece++) {
		int count = countBits(board->bitboards[PIECE(opponent, piece)]);
		material += count * fitnessPiece[piece];
	}
	return 1.0f - material / fitnessStartNoKing;
}

float Evaluation::fitnessKingSafety(Board *board, Color color, float endgameFraction) {
	Bitboard kings = board->bitboards[PIECE(color, KING)];
	int kingIndex = kingSquare(kings);
	if(kingIndex < 0) {
		return 0;
	}
	int distanceFromHomeRank = color == WHITE ? kingIndex / 8 : 7 - kingIndex / 8;
	float rankFitness = -fitnessKingRankFactor * distanceFromHomeRank * (0.6f - endgameFraction);
	float fileFitness = fitnessKingFile[kingIndex % 8] * (0.6f - endgameFraction);

	float openFilePenalty = 0;
	float pawnShieldPenalty = 0;
	if(endgameFraction < 0.7f) {
		int protectorsHome = 3;
		int protectorsOneStep = 0;
		int file = kingIndex % 8;
		Bitboard pawns = board->bitboards[PIECE(color, PAWN)];
		if(file <= 2) {
			protectorsHome = countBits(pawns & pawnShieldQueenside[color]);
			protectorsOneStep = countBits(pawns & pawnShieldQueensideForward[color]);
		} else if(file >= 5) {
			protectorsHome = countBits(pawns & pawnShieldKingside[color]);
			protectorsOneStep = countBits(pawns & pawnShieldKingsideForward[color]);
		}
		if(protectorsHome + protectorsOneStep == 2) {
			pawnShieldPenalty = 25.0f * protectorsHome + 50.0f * protectorsOneStep;
		} else if(protectorsHome + protectorsOneStep == 1) {
			pawnShieldPenalty = 50.0f * protectorsHome + 75.0f * protectorsOneStep;
		} else if(protectorsHome + protectorsOneStep == 0) {
			pawnShieldPenalty = 150.0f;
		}
		pawnShieldPenalty *= (1.0f - endgameFraction);

		if(file <= 2 || file >= 5) {
			if((pawns & BB_FILE[file]) == BB_EMPTY) {
				openFilePenalty = 150.0f * (1.0f - endgameFraction);
			}
		}
	}
	return rankFitness + fileFitness - pawnShieldPenalty - openFilePenalty;
}

float Evaluation::fitnessRookFiles(Board *board, Color color) {
	float result = 0;
	Bitboard rooks = board->bitboards[PIECE(color, ROOK)];
	Bitboard myPawns = board->bitboards[PIECE(color, PAWN)];
	Bitboard oppPawns = board->bitboards[PIECE(FLIP(color), PAWN)];
	while(rooks) {
		int rookIndex = popBit(&rooks);
		Bitboard rookFile = BB_FILE[rookIndex % 8];
		if((rookFile & myPawns) == BB_EMPTY) {
			if((rookFile & oppPawns) == BB_EMPTY) {
				result += fitnessRookOpenFile;
			} else {
				result += fitnessRookSemiOpenFile;
			}
		}
	}
	return result;
}

float Evaluation::fitnessCastleRights(Board *board, Color color, float endgameFraction) {
	if(endgameFraction > 0.5f) {
		return 0;
	}
	float result = 0;
	if(board->hasCastleRight((CastleRight) CASTLE_RIGHT(color, QUEENSIDE))) {
		result += fitnessCastleRightQueenside;
	}
	if(board->hasCastleRight((CastleRight) CASTLE_RIGHT(color, KINGSIDE))) {
		result += fitnessCastleRightKingside;
	}
	Bitboard pawns = board->bitboards[PIECE(color, PAWN)];
	int numPawnsQueenside = countBits(pawns & pawnShieldQueenside[color]);
	int numPawnsKingside = countBits(pawns & pawnShieldKingside[color]);
	result -= 10.0f * (3 - numPawnsQueenside);
	result -= 25.0f * (3 - numPawnsKingside);
	result *= (1.0f - 2.0f * endgameFraction);
	return result;
}

const PawnEntry *Evaluation::pawnEntry(Board *board) {
	U64 hash = board->positionHashPawnsKings;
	U64 whitePawns = board->bitboards[WHITE_PAWN];
	U64 blackPawns = board->bitboards[BLACK_PAWN];
	int kingWhite = kingSquare(board->bitboards[WHITE_KING]);
	int kingBlack = kingSquare(board->bitboards[BLACK_KING]);
	PawnEntry *entry = &pawnTable[hash & (PAWN_TABLE_SIZE - 1)];
	if(entry->used && entry->hash == hash && entry->whitePawns == whitePawns
			&& entry->blackPawns == blackPawns && entry->kingWhite == kingWhite
			&& entry->kingBlack == kingBlack) {
		return entry;
	}

	entry->used = true;
	entry->hash = hash;
	entry->whitePawns = whitePawns;
	entry->blackPawns = blackPawns;
	entry->kingWhite = kingWhite;
	entry->kingBlack = kingBlack;
	entry->doubledWhite = 0;
	entry->doubledBlack = 0;
	entry->isolatedWhite = 0;
	entry->isolatedBlack = 0;
	entry->passedWhite = 0;
	entry->passedBlack = 0;

	for(int file = 0; file < 8; file++) {
		Bitboard fileMask = BB_FILE[file];
		int filePawnsWhite = countBits(fileMask & whitePawns);
		int filePawnsBlack = countBits(fileMask & blackPawns);
		if(filePawnsWhite > 1) {
			entry->doubledWhite += filePawnsWhite;
		}
		if(filePawnsBlack > 1) {
			entry->doubledBlack += filePawnsBlack;
		}
		Bitboard neighbors = BB_EMPTY;
		if(file > 0) {
			neighbors |= BB_FILE[file - 1];
		}
		if(file < 7) {
			neighbors |= BB_FILE[file + 1];
		}
		if(filePawnsWhite >= 1 && countBits(neighbors & whitePawns) == 0) {
			entry->isolatedWhite += filePawnsWhite;
		}
		if(filePawnsBlack >= 1 && countBits(neighbors & blackPawns) == 0) {
			entry->isolatedBlack += filePawnsBlack;
		}
	}

	Bitboard white = whitePawns;
	while(white) {
		int pawnIndex = popBit(&white);
		if((passedPawnMasks[WHITE][pawnIndex] & blackPawns) == BB_EMPTY) {
			entry->passedWhite++;
		}
	}
	Bitboard black = blackPawns;
	while(black) {
		int pawnIndex = popBit(&black);
		if((passedPawnMasks[BLACK][pawnIndex] & whitePawns) == BB_EMPTY) {
			entry->passedBlack++;
		}
	}
	return entry;
}

float Evaluation::fitness(Board *board) {
	float score = 0;
	Color turn = board->turn;
	Color opponent = FLIP(turn);
	for(int piece = KNIGHT; piece <= KING; piece++) {
		int myCount = countBits(board->bitboards[PIECE(turn, piece)]);
		int oppCount = countBits(board->bitboards[PIECE(opponent, piece)]);
		score += (myCount - oppCount) * fitnessPiece[piece];
		if(piece == BISHOP) {
			if(myCount >= 2) {
				score += fitnessBishopPairBonus;
			}
			if(oppCount >= 2) {
				score -= fitnessBishopPairBonus;
			}
		}
	}

	float phase = endgameFraction(board);
	Bitboard myPawns = board->bitboards[PIECE(turn, PAWN)];
	Bitboard oppPawns = board->bitboards[PIECE(opponent, PAWN)];
	if(turn == WHITE) {
		oppPawns = flipRanks(oppPawns);
	} else {
		myPawns = flipRanks(myPawns);
	}

	const PawnEntry *entry = pawnEntry(board);
	float doubledPawnPenalty = 15.0f * (entry->doubledWhite - entry->doubledBlack);
	float isolatedPawnPenalty = 15.0f * (entry->isolatedWhite - entry->isolatedBlack);
	float passedPawnBonus = 30.0f * (entry->passedWhite - entry->passedBlack);
	if(turn == BLACK) {
		doubledPawnPenalty *= -1;
		isolatedPawnPenalty *= -1;
		passedPawnBonus *= -1;
	}
	score -= doubledPawnPenalty;
	score -= isolatedPawnPenalty;
	score += passedPawnBonus;

	for(int rank = 1; rank < 7; rank++) {
		Bitboard rankMask = BB_RANK[rank];
		for(int centrality = 0; centrality < 4; centrality++) {
			Bitboard centralityMask = BB_FILE[centrality] | BB_FILE[7 - centrality];
			float pawnFactor = (1.0f - phase) * fitnessPawnTableOpening[rank][centrality];
			pawnFactor += phase * fitnessPawnTableEndgame[rank][centrality];
			int myPawnsOnRank = countBits(myPawns & rankMask & centralityMask);
			int oppPawnsOnRank = countBits(oppPawns & rankMask & centralityMask);
			score += pawnFactor * (myPawnsOnRank - oppPawnsOnRank);
		}
	}

	score += fitnessKingSafety(board, turn, phase) - fitnessKingSafety(board, opponent, phase);
	score += fitnessRookFiles(board, turn) - fitnessRookFiles(board, opponent);
	score += fitnessCastleRights(board, turn, phase) - fitnessCastleRights(board, opponent, phase);

	if(board->bitboards[PIECE(turn, KING)] == BB_EMPTY) {
		score += board->ply * FITNESS_MOVE;
	}
	if(board->bitboards[PIECE(opponent, KING)] == BB_EMPTY) {
		score -= board->ply * FITNESS_MOVE;
	}
	return score;
}
