#include "brain.h"

Brain::Brain() {
	moveGen = MoveGen();
}

Move Brain::getMove(Board *board) {
	Move moves[MAX_MOVES];
	Move *movesCurrent = moves;
	Move *movesEnd = moveGen.legalMovesFast(board, movesCurrent);
	Move result = MOVE_NONE;
	while(movesCurrent != movesEnd) {
		if(!moveGen.isLegal(board, *movesCurrent)) {
			movesCurrent++;
			continue;
		}
		result = *movesCurrent;
		movesCurrent++;
	}
	return result;
}
