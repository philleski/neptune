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

struct SearchLimits {
	int maxDepth;
	bool useTime;
	int softMs;
	int hardMs;
	// No depth and no clock. Only stop or quit ends the search.
	bool untilStop;
};

typedef bool (*SearchStopFn)();
typedef void (*SearchInfoFn)(SearchResult result, long long nodes, int elapsedMs);

class Brain {
public:
	static constexpr int MAX_DEPTH = 64;

	Brain();
	void clear();
	void setHashMegabytes(int megabytes);
	int transpositionEntries() const;
	SearchResult getMove(Board *board, SearchLimits limits, SearchStopFn stop, SearchInfoFn info);
	float evaluate(Board *board);
	float kingSafety(Board *board, Color color, float endgameFraction);
	float endgameFraction(Board *board);
	int principalVariation(Board *board, Move move, Move *out, int maxMoves);
	static bool isMateScore(int score);
	static int mateMoves(int score);
private:
	static constexpr int MATE = 30000;
	static constexpr int INF = MATE + 1;
	// Leaves stay at or inside this. Mate scores start one past it.
	static constexpr int EVAL_CLAMP = MATE - MAX_DEPTH;
	static constexpr long long NODE_POLL = 2048;

	// A repetition or a fifty-move claim depends on the path, so it must not be stored.
	struct SearchScore {
		int score;
		bool pathDependent;
		bool aborted = false;
	};

	struct DepthResult {
		Move move;
		int score;
		bool aborted;
	};

	DepthResult getMoveToDepth(Board *board, int depth, Move previousBest);
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
	bool visitNode();
	bool clockHasRoom(long long elapsedMs, long long iterMs) const;
	void emitInfo(SearchResult result);

	MoveGen moveGen;
	Evaluation evaluation;
	TranspositionTable transpositionTable;
	Move killers[MAX_DEPTH][2];
	long long nodes;
	long long searchStartMs;
	bool searchAborted;
	bool enforceHard;
	bool useTime;
	int softMs;
	int hardMs;
	SearchStopFn pollStop;
	SearchInfoFn onInfo;
};

#endif /* SRC_BRAIN_H_ */
