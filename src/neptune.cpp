#include <iostream>
#include <stdio.h>
#include <time.h>
#include <sstream>
#include <sys/time.h>

#include "bitboard.h"
#include "board.h"
#include "movegen.h"

int perft(Board *board, int depth, MoveGen *moveGen) {
	int nodes = 0;
	if(depth == 0) {
		return 1;
	}
	Move moves[MAX_PLIES];
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
			std::cout << "Not implemented - go!!\n";
		} else if(token == "quit") {
			return 0;
		}
	}
	return 0;
}
