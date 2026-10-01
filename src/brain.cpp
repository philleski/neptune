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

float Brain::quiescentSearch(Board *board, float alpha, float beta, int target) {
	// Missing king: loss, with a ply penalty so shorter mates score better.
	if(board->bitboards[PIECE(board->turn, KING)] == BB_EMPTY) {
		return -Evaluation::FITNESS_LARGE + board->ply * Evaluation::FITNESS_MOVE;
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

float Brain::alphabeta(Board *board, int depth, float alpha, float beta) {
	// Missing king: loss, with a ply penalty so shorter mates score better.
	if(board->bitboards[PIECE(board->turn, KING)] == BB_EMPTY) {
		return -Evaluation::FITNESS_LARGE + board->ply * Evaluation::FITNESS_MOVE;
	}
	if(depth == 0) {
		return quiescentSearch(board, alpha, beta, -1);
	}

	TranspositionEntry entry;
	Move tableMove = MOVE_NONE;
	if(transpositionTable.get(board->positionHash, &entry)) {
		if(entry.depth == depth) {
			if(entry.type == TT_PV) {
				return entry.score;
			}
			if(entry.type == TT_CUT && entry.score > alpha) {
				alpha = entry.score;
			}
			if(alpha >= beta) {
				return beta;
			}
		}
		tableMove = entry.move;
	}

	Move moves[MAX_MOVES];
	int count = collectLegal(board, moves, false, depth, tableMove);
	if(count == 0) {
		if(moveGen.inCheck(board)) {
			// Checkmate. The ply penalty prefers the shorter mate.
			return -Evaluation::FITNESS_LARGE + board->ply * Evaluation::FITNESS_MOVE;
		}
		return 0;
	}

	int nodeType = TT_ALL;
	Move bestMove = MOVE_NONE;
	for(int i = 0; i < count; i++) {
		board->move(moves[i]);
		float score = -alphabeta(board, depth - 1, -beta, -alpha);
		board->unmove(moves[i]);
		if(score >= beta) {
			transpositionTable.put(board->positionHash, depth, beta, moves[i], TT_CUT);
			insertKiller(moves[i], depth);
			return beta;
		}
		if(score > alpha) {
			nodeType = TT_PV;
			bestMove = moves[i];
			alpha = score;
		}
	}
	transpositionTable.put(board->positionHash, depth, alpha, bestMove, nodeType);
	return alpha;
}

Move Brain::getMoveToDepth(Board *board, int depth, float *score) {
	unsetKillers();
	float alpha = -Evaluation::FITNESS_LARGE;
	float beta = Evaluation::FITNESS_LARGE;
	Move moves[MAX_MOVES];
	int count = collectLegal(board, moves, false, depth, MOVE_NONE);
	Move bestMove = MOVE_NONE;
	for(int i = 0; i < count; i++) {
		board->move(moves[i]);
		float fitness = -alphabeta(board, depth - 1, -beta, -alpha);
		board->unmove(moves[i]);
		if(bestMove == MOVE_NONE || fitness > alpha) {
			bestMove = moves[i];
			alpha = fitness;
		}
	}
	*score = alpha;
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
	for(int depth = 1; depth <= maxDepth; depth++) {
		float score = 0;
		Move move = getMoveToDepth(board, depth, &score);
		if(move == MOVE_NONE) {
			break;
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
