#ifndef SRC_BRAIN_H_
#define SRC_BRAIN_H_

#include "evaluation.h"
#include "movegen.h"
#include "transposition.h"
#include "types.h"

struct SearchResult {
	Move move;
	float score;
	int depth;
};

class Brain {
public:
	Brain();
	void clear();
	SearchResult getMove(Board *board, int maxDepth);
	float evaluate(Board *board);
	float kingSafety(Board *board, Color color, float endgameFraction);
	float endgameFraction(Board *board);
	int principalVariation(Board *board, Move move, Move *out, int maxMoves);
private:
	static const int MAX_DEPTH = 64;

	// A repetition or a fifty-move claim depends on the path, so it must not be stored.
	struct SearchScore {
		float score;
		bool pathDependent;
	};

	Move getMoveToDepth(Board *board, int depth, float *score, bool *pathDependent);
	SearchScore alphabeta(Board *board, int depth, float alpha, float beta);
	float quiescentSearch(Board *board, float alpha, float beta, int target);
	void storeScore(U64 hash, int depth, float score, Move move, int type, bool pathDependent);
	int collectLegal(Board *board, Move *moves, bool capturesOnly, int depth, Move ttMove);
	int collectTactical(Board *board, Move *moves);
	void orderMoves(Move *moves, int count, Board *board, int depth, Move ttMove);
	bool isKiller(Move move, int depth);
	void insertKiller(Move move, int depth);
	void unsetKillers();
	bool moveIsLegal(Board *board, Move move);

	MoveGen moveGen;
	Evaluation evaluation;
	TranspositionTable transpositionTable;
	Move killers[MAX_DEPTH][2];
};

#endif /* SRC_BRAIN_H_ */
