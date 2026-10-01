#include <iostream>
#include <sstream>
#include <sys/time.h>

#include "bitboard.h"
#include "board.h"
#include "brain.h"
#include "movegen.h"

int perft(Board *board, int depth, MoveGen *moveGen) {
	int nodes = 0;
	if(depth == 0) {
		return 1;
	}
	Move moves[MAX_MOVES];
	Move *movesCurrent = moves;
	Move *movesEnd = moveGen->generateMoves(board, movesCurrent);
	while(movesCurrent != movesEnd) {
		if(!moveGen->isLegal(board, *movesCurrent)) {
			movesCurrent++;
			continue;
		}
		if(depth == 1) {
			nodes++;
		} else {
			board->move(*movesCurrent);
			nodes += perft(board, depth - 1, moveGen);
			board->unmove(*movesCurrent);
		}
		movesCurrent++;
	}
	return nodes;
}

std::string moveToUci(Move move) {
	if(move == MOVE_NONE) {
		return "0000";
	}
	int source = SOURCE(move);
	int destination = DEST(move);
	std::string text;
	text += (char) ('a' + source % 8);
	text += (char) ('1' + source / 8);
	text += (char) ('a' + destination % 8);
	text += (char) ('1' + destination / 8);
	if(IS_PROMOTION(move)) {
		int promotion = move & 0x3000;
		if(promotion == MOVE_PROMOTE_TO_QUEEN) {
			text += 'q';
		} else if(promotion == MOVE_PROMOTE_TO_ROOK) {
			text += 'r';
		} else if(promotion == MOVE_PROMOTE_TO_BISHOP) {
			text += 'b';
		} else {
			text += 'n';
		}
	}
	return text;
}

Move findUciMove(Board *board, MoveGen *moveGen, const std::string &text) {
	if(text.size() < 4) {
		return MOVE_NONE;
	}
	int source = (text[0] - 'a') + 8 * (text[1] - '1');
	int destination = (text[2] - 'a') + 8 * (text[3] - '1');
	if(source < 0 || source > 63 || destination < 0 || destination > 63) {
		return MOVE_NONE;
	}
	char promotion = text.size() >= 5 ? text[4] : '\0';
	Move moves[MAX_MOVES];
	Move *end = moveGen->generateMoves(board, moves);
	for(Move *current = moves; current != end; current++) {
		if(SOURCE(*current) != source || DEST(*current) != destination) {
			continue;
		}
		if(!moveGen->isLegal(board, *current)) {
			continue;
		}
		if(promotion) {
			if(!IS_PROMOTION(*current)) {
				continue;
			}
			int piece = *current & 0x3000;
			if(promotion == 'q' && piece == MOVE_PROMOTE_TO_QUEEN) {
				return *current;
			}
			if(promotion == 'r' && piece == MOVE_PROMOTE_TO_ROOK) {
				return *current;
			}
			if(promotion == 'b' && piece == MOVE_PROMOTE_TO_BISHOP) {
				return *current;
			}
			if(promotion == 'n' && piece == MOVE_PROMOTE_TO_KNIGHT) {
				return *current;
			}
		} else if(!IS_PROMOTION(*current)) {
			return *current;
		}
	}
	return MOVE_NONE;
}

std::string readFen(std::istringstream *ss, std::string *token, bool *sawMoves) {
	std::string fen;
	*sawMoves = false;
	while(*ss >> *token) {
		if(*token == "moves") {
			*sawMoves = true;
			break;
		}
		fen += *token + " ";
	}
	return fen;
}

unsigned long long millisecondsSinceEpoch = 0ULL;

int elapsedMillis() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned long long millisecondsSinceEpochNew =
	    (unsigned long long)(tv.tv_sec) * 1000 +
	    (unsigned long long)(tv.tv_usec) / 1000;
	int diff = (int)(millisecondsSinceEpochNew - millisecondsSinceEpoch);
	millisecondsSinceEpoch = millisecondsSinceEpochNew;
	return diff;
}

int main() {
	Board board = Board();
	Brain brain = Brain();
	MoveGen moveGen = MoveGen();
	std::string line, token;

	initBitboards();

	while(std::getline(std::cin, line)) {
		std::istringstream ss(line);
		if(!(ss >> token)) {
			continue;
		}
		if(token == "uci") {
			std::cout << "id name Neptune" << std::endl;
			std::cout << "id author Phil Leszczynski" << std::endl;
			std::cout << "uciok" << std::endl;
		} else if(token == "isready") {
			std::cout << "readyok" << std::endl;
		} else if(token == "ucinewgame") {
			board = Board();
			brain.clear();
		} else if(token == "position") {
			bool sawMoves = false;
			ss >> token;
			if(token == "startpos") {
				board = Board();
				if(ss >> token) {
					sawMoves = token == "moves";
				}
			} else if(token == "fen") {
				std::string fen = readFen(&ss, &token, &sawMoves);
				board.setPosition(fen);
			}
			if(sawMoves) {
				while(ss >> token) {
					Move played = findUciMove(&board, &moveGen, token);
					if(played == MOVE_NONE) {
						break;
					}
					board.move(played);
				}
			}
		} else if(token == "_perft") {
			int depth;
			ss >> depth;
			std::string fen;
			while(ss >> token) {
				fen += token + " ";
			}
			board.setPosition(fen);
			elapsedMillis();
			std::cout << perft(&board, depth, &moveGen) << " " << elapsedMillis() << std::endl;
		} else if(token == "_eval") {
			std::string fen;
			while(ss >> token) {
				fen += token + " ";
			}
			board.setPosition(fen);
			std::cout << brain.evaluate(&board) << std::endl;
		} else if(token == "_king") {
			ss >> token;
			Color color = token == "b" ? BLACK : WHITE;
			float fraction;
			ss >> fraction;
			std::string fen;
			while(ss >> token) {
				fen += token + " ";
			}
			board.setPosition(fen);
			std::cout << brain.kingSafety(&board, color, fraction) << std::endl;
		} else if(token == "_eg") {
			std::string fen;
			while(ss >> token) {
				fen += token + " ";
			}
			board.setPosition(fen);
			std::cout << brain.endgameFraction(&board) << std::endl;
		} else if(token == "_hash") {
			std::cout << std::hex << board.positionHash << " " << board.positionHashPawnsKings
				<< std::dec << std::endl;
		} else if(token == "_roundtrip") {
			bool ok = board.hashesMatchRecompute();
			U64 full = board.positionHash;
			U64 pawns = board.positionHashPawnsKings;
			Move moves[MAX_MOVES];
			Move *end = moveGen.generateMoves(&board, moves);
			for(Move *current = moves; current != end; current++) {
				if(!moveGen.isLegal(&board, *current)) {
					continue;
				}
				board.move(*current);
				if(!board.hashesMatchRecompute()) {
					ok = false;
				}
				board.unmove(*current);
				if(board.positionHash != full || board.positionHashPawnsKings != pawns) {
					ok = false;
				}
			}
			std::cout << (ok ? "ok" : "fail") << std::endl;
		} else if(token == "go") {
			int depth = 6;
			while(ss >> token) {
				if(token == "depth") {
					ss >> depth;
				}
			}
			SearchResult result = brain.getMove(&board, depth);
			Move variation[64];
			int variationLength = brain.principalVariation(&board, result.move, variation,
				result.depth > 0 ? result.depth : 1);
			std::cout << "info depth " << result.depth << " score cp " << (int) result.score << " pv";
			for(int i = 0; i < variationLength; i++) {
				std::cout << " " << moveToUci(variation[i]);
			}
			std::cout << std::endl;
			std::cout << "bestmove " << moveToUci(result.move) << std::endl;
		} else if(token == "quit") {
			return 0;
		}
	}
	return 0;
}
