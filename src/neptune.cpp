
#include <stdio.h>
#include <time.h>
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
	initBitboards();
	MoveGen moveGen = MoveGen();
	Board board = Board();
	std::string fen;

	printf("*** Initial\n");
	difftime();
	for(int i = 0; i < 6; i++) {
		printf("Perft %d: %d\n", i, perft(&board, i, &moveGen));
	}
	printf("Time: %d ms\n", difftime());
	printf("\n");

	printf("*** Kiwi Pete\n");
	fen = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
	board.setPosition(fen);
	for(int i = 0; i < 5; i++) {
		printf("Perft %d: %d\n", i, perft(&board, i, &moveGen));
	}
	printf("Time: %d ms\n", difftime());
	printf("\n");

	printf("*** Position 3\n");
	fen = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
	board.setPosition(fen);
	for(int i = 0; i < 6; i++) {
		printf("Perft %d: %d\n", i, perft(&board, i, &moveGen));
	}
	printf("Time: %d ms\n", difftime());
	printf("\n");

	printf("*** Position 4\n");
	fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
	board.setPosition(fen);
	for(int i = 0; i < 5; i++) {
		printf("Perft %d: %d\n", i, perft(&board, i, &moveGen));
	}
	printf("Time: %d ms\n", difftime());
	printf("\n");

	printf("*** Position 4 Mirrored\n");
	fen = "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1";
	board.setPosition(fen);
	for(int i = 0; i < 5; i++) {
		printf("Perft %d: %d\n", i, perft(&board, i, &moveGen));
	}
	printf("Time: %d ms\n", difftime());
	printf("\n");

	printf("*** Position 5\n");
	fen = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
	board.setPosition(fen);
	for(int i = 0; i < 5; i++) {
		printf("Perft %d: %d\n", i, perft(&board, i, &moveGen));
	}
	printf("Time: %d ms\n", difftime());
	printf("\n");

	printf("*** Position 6\n");
	fen = "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10";
	board.setPosition(fen);
	for(int i = 0; i < 5; i++) {
		printf("Perft %d: %d\n", i, perft(&board, i, &moveGen));
	}
	printf("Time: %d ms\n", difftime());
	printf("\n");

	return 0;
}
