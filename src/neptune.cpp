#include <iostream>
#include <stdio.h>
#include <time.h>
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
	Move *movesEnd = moveGen->legalMovesFast(board, movesCurrent);
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

unsigned long long millisecondsSinceEpoch = 0ULL;

int difftime() {
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

	while(1) {
		std::getline(std::cin, line);
		std::istringstream ss(line);
		ss >> std::skipws >> token;
		if(token == "uci") {
			std::cout << "id name Neptune\n";
			std::cout << "id author Phil Leszczynski\n";
			std::cout << "uciok\n";
		} else if(token == "isready") {
			std::cout << "readyok\n";
		} else if(token == "position") {
			ss >> token;
			if(token == "fen") {
				std::string fen;
				while(ss >> token) {
					fen += token + " ";
				}
				board.setPosition(fen);
			}
		} else if(token == "_perft") {
			int depth;
			ss >> depth;
			std::string fen;
			while(ss >> token) {
				fen += token + " ";
			}
			board.setPosition(fen);
			difftime();
			std::cout << perft(&board, depth, &moveGen) << " " << difftime() << std::endl;
		} else if(token == "go") {
			Move move = brain.getMove(&board);
			int source = move & 63;
			int destination = (move >> 6) & 63;
			char sourceFile = (source % 8) + 'a';
			char sourceRank = (source / 8) + '1';
			char destinationFile = (destination % 8) + 'a';
			char destinationRank = (destination / 8) + '1';
			char promotionPiece = '\0';
			if(move & MOVE_PROMOTION) {
				// Don't check the knight promotion because it's 0.
				promotionPiece = 'n';
				if(move & MOVE_PROMOTE_TO_BISHOP) {
					promotionPiece = 'b';
				} else if(move & MOVE_PROMOTE_TO_ROOK) {
					promotionPiece = 'r';
				} else if(move & MOVE_PROMOTE_TO_QUEEN) {
					promotionPiece = 'q';
				}
			}
			std::cout << "bestmove " << sourceFile << sourceRank << destinationFile << destinationRank << promotionPiece << std::endl;
		} else if(token == "quit") {
			return 0;
		}
	}
	return 0;
}
