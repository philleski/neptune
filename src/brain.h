#ifndef SRC_BRAIN_H_
#define SRC_BRAIN_H_

#include "evaluation.h"
#include "movegen.h"
#include "transposition.h"
#include "types.h"

struct SearchResult {
	Move move;
	int score;
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
	static bool isMateScore(int score);
	static int mateMoves(int score);
private:
	static constexpr int MAX_DEPTH = 64;
	static constexpr int MATE = 30000;
	static constexpr int INF = MATE + 1;
	// Leaves stay at or inside this. Mate scores start one past it.
	static constexpr int EVAL_CLAMP = MATE - MAX_DEPTH;

	// A repetition or a fifty-move claim depends on the path, so it must not be stored.
	struct SearchScore {
		int score;
		bool pathDependent;
	};

	Move getMoveToDepth(Board *board, int depth, int *score, bool *pathDependent);
	SearchScore alphabeta(Board *board, int depth, int alpha, int beta);
	int quiescentSearch(Board *board, int alpha, int beta, int target);
	int leafScore(Board *board);
	static int backup(int childScore);
	void storeScore(U64 hash, int depth, int score, Move move, int type, bool pathDependent);
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
