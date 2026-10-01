#ifndef SRC_EVALUATION_H_
#define SRC_EVALUATION_H_

#include <vector>

#include "board.h"

struct PawnEntry {
	U64 hash;
	U64 whitePawns;
	U64 blackPawns;
	int doubledWhite;
	int doubledBlack;
	int isolatedWhite;
	int isolatedBlack;
	int passedWhite;
	int passedBlack;
	int kingWhite;
	int kingBlack;
	bool used;
};

// Fitness is the static evaluation score.
class Evaluation {
public:
	Evaluation();
	float fitness(Board *board);
	float endgameFraction(Board *board);
	float fitnessKingSafety(Board *board, Color color, float endgameFraction);

	static const float FITNESS_LARGE;
	static const float FITNESS_MOVE;
private:
	const PawnEntry *pawnEntry(Board *board);
	float fitnessRookFiles(Board *board, Color color);
	float fitnessCastleRights(Board *board, Color color, float endgameFraction);

	static const int PAWN_TABLE_SIZE = 64 * 1024;
	std::vector<PawnEntry> pawnTable;
	Bitboard passedPawnMasks[2][64];
	Bitboard pawnShieldQueenside[2];
	Bitboard pawnShieldQueensideForward[2];
	Bitboard pawnShieldKingside[2];
	Bitboard pawnShieldKingsideForward[2];

	float fitnessPiece[6];
	float fitnessStartNoKing;
	float fitnessRookOpenFile;
	float fitnessRookSemiOpenFile;
	float fitnessCastleRightQueenside;
	float fitnessCastleRightKingside;
	float fitnessBishopPairBonus;
	float fitnessPawnTableOpening[8][4];
	float fitnessPawnTableEndgame[8][4];
	float fitnessKingRankFactor;
	float fitnessKingFile[8];
};

#endif /* SRC_EVALUATION_H_ */
