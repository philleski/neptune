#include "brain.h"

#include <cmath>
#include <sys/time.h>

static long long nowMillis() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (long long) tv.tv_sec * 1000 + (long long) tv.tv_usec / 1000;
}

Brain::Brain() {
	unsetKillers();
	nodes = 0;
	searchStartMs = 0;
	searchAborted = false;
	enforceHard = false;
	useTime = false;
	softMs = 0;
	hardMs = 0;
	pollStop = nullptr;
	onInfo = nullptr;
}

void Brain::clear() {
	transpositionTable.clear();
	unsetKillers();
}

void Brain::setHashMegabytes(int megabytes) {
	transpositionTable.resize(megabytes);
}

int Brain::transpositionEntries() const {
	return transpositionTable.entries();
}

float Brain::evaluate(Board *board) {
	return evaluation.fitness(board);
}

float Brain::kingSafety(Board *board, Color color, float endgameFraction) {
	return evaluation.fitnessKingSafety(board, color, endgameFraction);
}

float Brain::endgameFraction(Board *board) {
	return evaluation.endgameFraction(board);
}

void Brain::unsetKillers() {
	for(int depth = 0; depth < MAX_DEPTH; depth++) {
		killers[depth][0] = MOVE_NONE;
		killers[depth][1] = MOVE_NONE;
	}
}

bool Brain::visitNode() {
	nodes++;
	if(searchAborted) {
		return true;
	}
	// The hard limit starts at depth 2. Stop is checked on every poll.
	if((nodes & (NODE_POLL - 1)) != 0) {
		return false;
	}
	if(enforceHard && nowMillis() - searchStartMs >= hardMs) {
		searchAborted = true;
		return true;
	}
	if(pollStop != nullptr && pollStop()) {
		searchAborted = true;
		return true;
	}
	return false;
}

bool Brain::clockHasRoom(long long elapsedMs, long long iterMs) const {
	if(!useTime) {
		return true;
	}
	if(elapsedMs >= softMs) {
		return false;
	}
	// The next depth takes at least as long as this one.
	if(iterMs >= softMs - elapsedMs) {
		return false;
	}
	return true;
}

void Brain::emitInfo(SearchResult result) {
	if(onInfo == nullptr) {
		return;
	}
	long long elapsed = nowMillis() - searchStartMs;
	if(elapsed < 0) {
		elapsed = 0;
	}
	onInfo(result, nodes, (int) elapsed);
}

bool Brain::isKiller(Move move, int depth) {
	if(depth < 0 || depth >= MAX_DEPTH) {
		return false;
	}
	return killers[depth][0] == move || killers[depth][1] == move;
}

void Brain::insertKiller(Move move, int depth) {
	if(depth < 0 || depth >= MAX_DEPTH || isKiller(move, depth)) {
		return;
	}
	killers[depth][0] = killers[depth][1];
	killers[depth][1] = move;
}

void Brain::orderMoves(Move *moves, int count, Board *board, int depth, Move ttMove) {
	Move tableMoves[MAX_MOVES];
	Move captures[MAX_MOVES];
	Move killerMoves[MAX_MOVES];
	Move quietMoves[MAX_MOVES];
	int tableCount = 0;
	int captureCount = 0;
	int killerCount = 0;
	int quietCount = 0;
	for(int i = 0; i < count; i++) {
		Move move = moves[i];
		if(ttMove != MOVE_NONE && move == ttMove) {
			tableMoves[tableCount++] = move;
		} else if(board->isCapture(move)) {
			captures[captureCount++] = move;
		} else if(isKiller(move, depth)) {
			killerMoves[killerCount++] = move;
		} else {
			quietMoves[quietCount++] = move;
		}
	}
	int index = 0;
	for(int i = 0; i < tableCount; i++) {
		moves[index++] = tableMoves[i];
	}
	for(int i = 0; i < captureCount; i++) {
		moves[index++] = captures[i];
	}
	for(int i = 0; i < killerCount; i++) {
		moves[index++] = killerMoves[i];
	}
	for(int i = 0; i < quietCount; i++) {
		moves[index++] = quietMoves[i];
	}
}

static bool isCaptureMove(Board *board, Move move) {
	// En passant leaves the destination empty, so isCapture misses it.
	return board->isCapture(move) || IS_ENPASSANT(move);
}

int Brain::collectLegal(Board *board, Move *moves, bool capturesOnly, int depth, Move ttMove) {
	Move pseudo[MAX_MOVES];
	Move *end = moveGen.generateMoves(board, pseudo, capturesOnly);
	int count = 0;
	for(Move *current = pseudo; current != end; current++) {
		if(moveGen.isLegal(board, *current)) {
			moves[count++] = *current;
		}
	}
	orderMoves(moves, count, board, depth, ttMove);
	return count;
}

int Brain::collectTactical(Board *board, Move *moves) {
	// Captures, en passant, and every promotion. Quiets stay out of the horizon.
	Move pseudo[MAX_MOVES];
	Move *end = moveGen.generateMoves(board, pseudo, false);
	int count = 0;
	for(Move *current = pseudo; current != end; current++) {
		Move move = *current;
		if(!isCaptureMove(board, move) && !IS_PROMOTION(move)) {
			continue;
		}
		if(moveGen.isLegal(board, move)) {
			moves[count++] = move;
		}
	}
	orderMoves(moves, count, board, 0, MOVE_NONE);
	return count;
}

bool Brain::moveIsLegal(Board *board, Move move) {
	Move pseudo[MAX_MOVES];
	Move *end = moveGen.generateMoves(board, pseudo, false);
	for(Move *current = pseudo; current != end; current++) {
		if(*current == move && moveGen.isLegal(board, move)) {
			return true;
		}
	}
	return false;
}

void Brain::storeScore(U64 hash, int depth, int score, Move move, int type, bool pathDependent) {
	if(pathDependent) {
		return;
	}
	transpositionTable.put(hash, depth, score, move, type);
}

bool Brain::isMateScore(int score) {
	return score > EVAL_CLAMP || score < -EVAL_CLAMP;
}

int Brain::mateMoves(int score) {
	if(score > 0) {
		return (MATE - score + 1) / 2;
	}
	return -(MATE + score) / 2;
}

int Brain::backup(int childScore) {
	int score = -childScore;
	// Mate distance is plies from this node. Stepping toward zero makes a
	// mate one ply longer for the parent, so a shorter mate still scores higher.
	if(score > EVAL_CLAMP) {
		score--;
	} else if(score < -EVAL_CLAMP) {
		score++;
	}
	return score;
}

int Brain::leafScore(Board *board) {
	float raw = evaluation.fitness(board);
	if(raw >= EVAL_CLAMP) {
		return EVAL_CLAMP;
	}
	if(raw <= -EVAL_CLAMP) {
		return -EVAL_CLAMP;
	}
	return (int) std::lround(raw);
}

int Brain::quiescentSearch(Board *board, int alpha, int beta, int target) {
	if(visitNode()) {
		return 0;
	}
	// Missing king is a loss at this node. Distance is not the game ply.
	if(board->bitboards[PIECE(board->turn, KING)] == BB_EMPTY) {
		return -MATE;
	}
	// A dead position cannot be mate. Recognize it before the check test.
	if(board->isDeadPosition()) {
		return 0;
	}
	bool inCheck = moveGen.inCheck(board);
	Move moves[MAX_MOVES];
	int count;
	// In check there is no stand-pat. The best evasion is filled in below.
	int bestScore = -INF;
	if(inCheck) {
		// A check is a forced move. Stand-pat would score an illegal pass.
		count = collectLegal(board, moves, false, 0, MOVE_NONE);
		if(count == 0) {
			return -MATE;
		}
	} else {
		count = collectTactical(board, moves);
		if(count == 0) {
			// No capture and no promotion. An empty legal list is stalemate.
			// Checked before stand-pat, which would fail high on a positive eval.
			Move any[MAX_MOVES];
			if(collectLegal(board, any, false, 0, MOVE_NONE) == 0) {
				return 0;
			}
		}
		bestScore = leafScore(board);
		if(bestScore >= beta) {
			return bestScore;
		}
		if(bestScore > alpha) {
			alpha = bestScore;
		}
	}
	for(int i = 0; i < count; i++) {
		Move move = moves[i];
		bool capture = isCaptureMove(board, move);
		// Recaptures only, and only on the square of the capture just made.
		// A quiet promotion is not a recapture, so the square filter leaves it in.
		// target == -1 accepts any capture. In check the filter does not apply.
		if(!inCheck && capture && target != -1 && (int)DEST(move) != target) {
			continue;
		}
		// A capture continues the recapture chain. A quiet move did not take
		// anything, so the reply may capture on any square.
		int childTarget = capture ? (int)DEST(move) : -1;
		board->move(move);
		int score = backup(quiescentSearch(board, -beta, -alpha, childTarget));
		board->unmove(move);
		if(searchAborted) {
			return 0;
		}
		if(score > bestScore) {
			bestScore = score;
		}
		if(score >= beta) {
			return score;
		}
		if(score > alpha) {
			alpha = score;
		}
	}
	return bestScore;
}

Brain::SearchScore Brain::alphabeta(Board *board, int depth, int alpha, int beta) {
	if(visitNode()) {
		return {0, false, true};
	}
	// Missing king is a loss at this node. Distance is not the game ply.
	if(board->bitboards[PIECE(board->turn, KING)] == BB_EMPTY) {
		storeScore(board->positionHash, depth, -MATE, MOVE_NONE, TT_PV, false);
		return {-MATE, false};
	}
	// Repetition is path-dependent. Probe it before the table and do not store it.
	if(board->isRepetition()) {
		return {0, true};
	}
	if(board->isDeadPosition()) {
		storeScore(board->positionHash, depth, 0, MOVE_NONE, TT_PV, false);
		return {0, false};
	}

	// Fifty quiet moves have been made. The side to move can claim a draw.
	// A mating move still wins, and a checkmate already on the board is still a loss.
	bool claimDraw = board->halfmoveClock >= 100;
	if(depth == 0) {
		int stand = quiescentSearch(board, alpha, beta, -1);
		if(searchAborted) {
			return {0, false, true};
		}
		// The claim draws a lost horizon. A mate loss stays a loss.
		if(claimDraw && stand < 0 && !isMateScore(stand)) {
			return {0, true};
		}
		return {stand, false};
	}

	TranspositionEntry entry;
	Move tableMove = MOVE_NONE;
	if(transpositionTable.get(board->positionHash, &entry)) {
		// A stored score ignores the clock. At the claim limit, search instead.
		if(!claimDraw && entry.depth >= depth) {
			if(entry.type == TT_PV) {
				return {entry.score, false};
			}
			if(entry.type == TT_CUT) {
				if(entry.score >= beta) {
					return {entry.score, false};
				}
				if(entry.score > alpha) {
					alpha = entry.score;
				}
			} else if(entry.type == TT_ALL) {
				if(entry.score <= alpha) {
					return {entry.score, false};
				}
				if(entry.score < beta) {
					beta = entry.score;
				}
			}
		}
		tableMove = entry.move;
	}

	Move moves[MAX_MOVES];
	int count = collectLegal(board, moves, false, depth, tableMove);
	if(count == 0) {
		if(moveGen.inCheck(board)) {
			storeScore(board->positionHash, depth, -MATE, MOVE_NONE, TT_PV, false);
			return {-MATE, false};
		}
		storeScore(board->positionHash, depth, 0, MOVE_NONE, TT_PV, false);
		return {0, false};
	}
	if(claimDraw && 0 >= beta) {
		return {0, true};
	}
	int originalAlpha = alpha;
	if(claimDraw && alpha < 0) {
		alpha = 0;
	}

	int bestScore = -INF;
	Move bestMove = MOVE_NONE;
	bool bestPathDependent = true;
	for(int i = 0; i < count; i++) {
		board->move(moves[i]);
		SearchScore child = alphabeta(board, depth - 1, -beta, -alpha);
		board->unmove(moves[i]);
		if(child.aborted) {
			return {0, false, true};
		}
		int score = backup(child.score);
		if(score > bestScore) {
			bestScore = score;
			bestMove = moves[i];
			bestPathDependent = child.pathDependent;
		}
		if(score >= beta) {
			storeScore(board->positionHash, depth, score, moves[i], TT_CUT, child.pathDependent);
			insertKiller(moves[i], depth);
			return {score, child.pathDependent};
		}
		if(score > alpha) {
			alpha = score;
		}
	}
	// Every move loses, and the side to move can take the draw instead.
	if(claimDraw && bestScore < 0) {
		return {0, true};
	}
	int nodeType = bestScore <= originalAlpha ? TT_ALL : TT_PV;
	storeScore(board->positionHash, depth, bestScore, bestMove, nodeType, bestPathDependent);
	return {bestScore, bestPathDependent};
}

Brain::DepthResult Brain::getMoveToDepth(Board *board, int depth, Move previousBest) {
	int alpha = -INF;
	int beta = INF;
	// The previous depth's move leads. Depth 1 has none, so a move stored
	// from an earlier search of this position leads instead. The stored score
	// does not cut the root off, and the root is not written back.
	Move orderMove = previousBest;
	if(orderMove == MOVE_NONE) {
		TranspositionEntry entry;
		if(transpositionTable.get(board->positionHash, &entry)) {
			orderMove = entry.move;
		}
	}
	Move moves[MAX_MOVES];
	int count = collectLegal(board, moves, false, depth, orderMove);
	DepthResult result;
	result.move = MOVE_NONE;
	result.score = 0;
	result.aborted = false;
	if(count == 0) {
		result.score = moveGen.inCheck(board) ? -MATE : 0;
		return result;
	}
	Move bestMove = MOVE_NONE;
	int bestScore = -INF;
	for(int i = 0; i < count; i++) {
		board->move(moves[i]);
		SearchScore child = alphabeta(board, depth - 1, -beta, -alpha);
		board->unmove(moves[i]);
		if(child.aborted) {
			result.aborted = true;
			if(bestMove == MOVE_NONE) {
				result.move = moves[0];
				result.score = 0;
			} else {
				result.move = bestMove;
				result.score = bestScore;
			}
			return result;
		}
		int fitness = backup(child.score);
		if(bestMove == MOVE_NONE || fitness > bestScore) {
			bestMove = moves[i];
			bestScore = fitness;
			alpha = fitness;
		}
	}
	result.move = bestMove;
	result.score = bestScore;
	// The root window is not a table entry. Path-dependent child scores were
	// already refused by storeScore.
	return result;
}

SearchResult Brain::getMove(Board *board, SearchLimits limits, SearchStopFn stop, SearchInfoFn info) {
	if(limits.maxDepth < 1) {
		limits.maxDepth = 1;
	}
	if(limits.maxDepth > MAX_DEPTH - 1) {
		limits.maxDepth = MAX_DEPTH - 1;
	}
	unsetKillers();
	nodes = 0;
	searchAborted = false;
	enforceHard = false;
	useTime = limits.useTime;
	softMs = limits.softMs;
	hardMs = limits.hardMs;
	pollStop = stop;
	onInfo = info;
	searchStartMs = nowMillis();

	SearchResult result;
	result.move = MOVE_NONE;
	result.score = 0;
	result.depth = 0;
	// Already drawn. Fifty-move is searched below, so a mate in one is still played.
	if(board->isRepetition() || board->isDeadPosition()) {
		Move moves[MAX_MOVES];
		int count = collectLegal(board, moves, false, 0, MOVE_NONE);
		if(count > 0) {
			result.move = moves[0];
		}
		emitInfo(result);
		return result;
	}
	Move previousBest = MOVE_NONE;
	long long iterMs = 0;
	for(int depth = 1; depth <= limits.maxDepth; depth++) {
		// The clock waits out depth 1. Stop is checked before every depth, so a
		// stop that is already buffered does not have to wait for 2048 nodes.
		if(depth > 1 && !clockHasRoom(nowMillis() - searchStartMs, iterMs)) {
			break;
		}
		if(pollStop != nullptr && pollStop()) {
			if(result.depth == 0) {
				Move moves[MAX_MOVES];
				int count = collectLegal(board, moves, false, 0, MOVE_NONE);
				if(count > 0) {
					result.move = moves[0];
				}
				result.score = 0;
				result.depth = depth;
				emitInfo(result);
			}
			break;
		}
		enforceHard = useTime && depth > 1;
		searchAborted = false;
		long long iterStart = nowMillis();
		DepthResult found = getMoveToDepth(board, depth, previousBest);
		iterMs = nowMillis() - iterStart;
		if(found.aborted) {
			// A later depth keeps the finished result. Depth 1 has none yet.
			if(result.depth == 0) {
				result.move = found.move;
				result.score = found.score;
				result.depth = depth;
				emitInfo(result);
			}
			break;
		}
		result.move = found.move;
		result.score = found.score;
		result.depth = depth;
		emitInfo(result);
		if(found.move == MOVE_NONE) {
			break;
		}
		previousBest = found.move;
	}
	return result;
}

int Brain::principalVariation(Board *board, Move move, Move *out, int maxMoves) {
	if(move == MOVE_NONE || maxMoves <= 0) {
		return 0;
	}
	int count = 0;
	out[count++] = move;
	board->move(move);
	while(count < maxMoves) {
		TranspositionEntry entry;
		// An all-node move orders the search. It is not part of the line.
		if(!transpositionTable.get(board->positionHash, &entry) || entry.type != TT_PV
				|| entry.move == MOVE_NONE) {
			break;
		}
		Move next = entry.move;
		if(!moveIsLegal(board, next)) {
			break;
		}
		out[count++] = next;
		board->move(next);
	}
	for(int i = count - 1; i >= 0; i--) {
		board->unmove(out[i]);
	}
	return count;
}
