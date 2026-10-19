#include "brain.h"

Brain::Brain() {
	unsetKillers();
}

void Brain::clear() {
	transpositionTable.clear();
	unsetKillers();
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

void Brain::storeScore(U64 hash, int depth, float score, Move move, int type, bool pathDependent) {
	if(pathDependent) {
		return;
	}
	transpositionTable.put(hash, depth, score, move, type);
}

float Brain::quiescentSearch(Board *board, float alpha, float beta, int target) {
	// Missing king: loss, with a ply penalty so shorter mates score better.
	if(board->bitboards[PIECE(board->turn, KING)] == BB_EMPTY) {
		return -Evaluation::FITNESS_LARGE + board->ply * Evaluation::FITNESS_MOVE;
	}
	if(board->isDeadPosition()) {
		return 0;
	}
	float standPat = evaluation.fitness(board);
	if(standPat >= beta) {
		return beta;
	}
	if(standPat > alpha) {
		alpha = standPat;
	}
	Move moves[MAX_MOVES];
	int count = collectLegal(board, moves, true, 0, MOVE_NONE);
	for(int i = 0; i < count; i++) {
		int destination = DEST(moves[i]);
		// Recaptures only, and only on the square of the capture just made.
		// target == -1 accepts any capture.
		if(target != -1 && destination != target) {
			continue;
		}
		board->move(moves[i]);
		float score = -quiescentSearch(board, -beta, -alpha, destination);
		board->unmove(moves[i]);
		if(score >= beta) {
			return beta;
		}
		if(score > alpha) {
			alpha = score;
		}
	}
	return alpha;
}

Brain::SearchScore Brain::alphabeta(Board *board, int depth, float alpha, float beta) {
	// Missing king: loss, with a ply penalty so shorter mates score better.
	if(board->bitboards[PIECE(board->turn, KING)] == BB_EMPTY) {
		float loss = -Evaluation::FITNESS_LARGE + board->ply * Evaluation::FITNESS_MOVE;
		return {loss, false};
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
		// The horizon does not otherwise notice mate. A claim must not hide one.
		if(claimDraw && moveGen.inCheck(board)) {
			Move moves[MAX_MOVES];
			int count = collectLegal(board, moves, false, depth, MOVE_NONE);
			if(count == 0) {
				float loss = -Evaluation::FITNESS_LARGE + board->ply * Evaluation::FITNESS_MOVE;
				return {loss, false};
			}
		}
		float stand = quiescentSearch(board, alpha, beta, -1);
		if(claimDraw && stand < 0) {
			return {0, true};
		}
		return {stand, false};
	}

	TranspositionEntry entry;
	Move tableMove = MOVE_NONE;
	if(transpositionTable.get(board->positionHash, &entry)) {
		// A stored score ignores the clock. At the claim limit, search instead.
		if(!claimDraw && entry.depth == depth) {
			if(entry.type == TT_PV) {
				return {entry.score, false};
			}
			if(entry.type == TT_CUT && entry.score > alpha) {
				alpha = entry.score;
			}
			if(alpha >= beta) {
				return {beta, false};
			}
		}
		tableMove = entry.move;
	}

	Move moves[MAX_MOVES];
	int count = collectLegal(board, moves, false, depth, tableMove);
	if(count == 0) {
		if(moveGen.inCheck(board)) {
			// Checkmate. The ply penalty prefers the shorter mate.
			float loss = -Evaluation::FITNESS_LARGE + board->ply * Evaluation::FITNESS_MOVE;
			return {loss, false};
		}
		return {0, false};
	}
	if(claimDraw && 0 >= beta) {
		return {0, true};
	}
	if(claimDraw && alpha < 0) {
		alpha = 0;
	}

	int nodeType = TT_ALL;
	Move bestMove = MOVE_NONE;
	// Until a move beats the claim, the floor of 0 depends on the path.
	bool bestPathDependent = claimDraw;
	for(int i = 0; i < count; i++) {
		board->move(moves[i]);
		SearchScore child = alphabeta(board, depth - 1, -beta, -alpha);
		board->unmove(moves[i]);
		float score = -child.score;
		if(score >= beta) {
			storeScore(board->positionHash, depth, beta, moves[i], TT_CUT, child.pathDependent);
			insertKiller(moves[i], depth);
			return {beta, child.pathDependent};
		}
		if(score > alpha) {
			nodeType = TT_PV;
			bestMove = moves[i];
			alpha = score;
			bestPathDependent = child.pathDependent;
		}
	}
	storeScore(board->positionHash, depth, alpha, bestMove, nodeType, bestPathDependent);
	return {alpha, bestPathDependent};
}

Move Brain::getMoveToDepth(Board *board, int depth, float *score, bool *pathDependent) {
	unsetKillers();
	float alpha = -Evaluation::FITNESS_LARGE;
	float beta = Evaluation::FITNESS_LARGE;
	Move moves[MAX_MOVES];
	int count = collectLegal(board, moves, false, depth, MOVE_NONE);
	Move bestMove = MOVE_NONE;
	bool bestPathDependent = false;
	for(int i = 0; i < count; i++) {
		board->move(moves[i]);
		SearchScore child = alphabeta(board, depth - 1, -beta, -alpha);
		board->unmove(moves[i]);
		float fitness = -child.score;
		if(bestMove == MOVE_NONE || fitness > alpha) {
			bestMove = moves[i];
			alpha = fitness;
			bestPathDependent = child.pathDependent;
		}
	}
	*score = alpha;
	*pathDependent = bestPathDependent;
	// The root window is not a table entry. Path-dependent child scores were
	// already refused by storeScore.
	return bestMove;
}

SearchResult Brain::getMove(Board *board, int maxDepth) {
	if(maxDepth < 1) {
		maxDepth = 1;
	}
	if(maxDepth > MAX_DEPTH - 1) {
		maxDepth = MAX_DEPTH - 1;
	}
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
		return result;
	}
	for(int depth = 1; depth <= maxDepth; depth++) {
		float score = 0;
		bool pathDependent = false;
		Move move = getMoveToDepth(board, depth, &score, &pathDependent);
		if(move == MOVE_NONE) {
			break;
		}
		// The root window is not a table entry. A path-dependent score is still
		// the score we report; storeScore already dropped it inside the tree.
		if(pathDependent) {
			result.score = score;
		}
		result.move = move;
		result.score = score;
		result.depth = depth;
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
		if(!transpositionTable.get(board->positionHash, &entry) || entry.move == MOVE_NONE) {
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
