#include <stdio.h>

#include "bitboard.h"
#include "board.h"
#include "movegen.h"
#include "types.h"

const char *printMove(Move move) {
	int squareSource = SOURCE(move);
	int squareDest = DEST(move);
	static char result[6];
	result[0] = FILE(squareSource);
	result[1] = RANK(squareSource);
	result[2] = '-';
	result[3] = FILE(squareDest);
	result[4] = RANK(squareDest);
	result[5] = '\0';
	return result;
}

MoveGen::MoveGen() {
	initAttackSquaresPawn(WHITE);
	initAttackSquaresPawn(BLACK);
	int stepSizesKnight[] = {-17, -15, -10, -6, 6, 10, 15, 17};
	initAttackSquaresShortRange(stepSizesKnight, attackSquaresKnight);
	int stepSizesKing[] = {-9, -8, -7, -1, 1, 7, 8, 9};
	initAttackSquaresShortRange(stepSizesKing, attackSquaresKing);

	castleBlockers[WHITE_KINGSIDE] = BB_SET[F1] | BB_SET[G1];
	castleBlockers[WHITE_QUEENSIDE] = BB_SET[D1] | BB_SET[C1] | BB_SET[B1];
	castleBlockers[BLACK_KINGSIDE] = BB_SET[F8] | BB_SET[G8];
	castleBlockers[BLACK_QUEENSIDE] = BB_SET[D8] | BB_SET[C8] | BB_SET[B8];

	memcpy(castlePath[WHITE_KINGSIDE], (int []){E1, F1, G1},
		sizeof castlePath[WHITE_KINGSIDE]);
	memcpy(castlePath[WHITE_QUEENSIDE], (int []){E1, D1, C1},
		sizeof castlePath[WHITE_QUEENSIDE]);
	memcpy(castlePath[BLACK_KINGSIDE], (int []){E8, F8, G8},
		sizeof castlePath[BLACK_KINGSIDE]);
	memcpy(castlePath[BLACK_QUEENSIDE], (int []){E8, D8, C8},
		sizeof castlePath[BLACK_QUEENSIDE]);
}

void MoveGen::initAttackSquaresPawn(Color color) {
	for(int square = A1; square <= H8; square++) {
		attackSquaresPawnMove[color][square] = BB_EMPTY;
		attackSquaresPawnCapture[color][square] = BB_EMPTY;
		if(color == WHITE && square < A2) {
			continue;
		}
		if(color == BLACK && square > H7) {
			continue;
		}
		attackSquaresPawnMove[color][square] |=
			BB_SET[square + FORWARD(color)];
		if(color == WHITE && square < A3) {
			attackSquaresPawnMove[color][square] |=
				BB_SET[square + FORWARD2(color)];
		}
		if(color == BLACK && square > H6) {
			attackSquaresPawnMove[color][square] |=
				BB_SET[square + FORWARD2(color)];
		}
		if(inBounds(square, FORWARD_WEST(square))) {
			attackSquaresPawnCapture[color][square] |=
				BB_SET[square + FORWARD_WEST(color)];
		}
		if(inBounds(square, FORWARD_EAST(square))) {
			attackSquaresPawnCapture[color][square] |=
				BB_SET[square + FORWARD_EAST(color)];
		}
	}
}

void MoveGen::initAttackSquaresShortRange(int stepSizes[],
		Bitboard *attackSquares) {
	for(int square = A1; square <= H8; square++) {
		attackSquares[square] = BB_EMPTY;
		for(int stepIndex = 0; stepIndex < 8; stepIndex++) {
			int stepSize = stepSizes[stepIndex];
			if(inBounds(square, stepSize)) {
				attackSquares[square] |= BB_SET[square + stepSize];
			}
		}
	}
}

Move *MoveGen::appendMovesForPawn(Board *board, Move *moves) {
	Bitboard bitboard = board->bitboards[PIECE(board->turn, PAWN)];
	while(bitboard) {
		int mover = popBit(&bitboard);
		bool isPromotable = (board->turn == WHITE ? mover >= 48 : mover < 16);
		// If the pawn is trying to move two squares up and there's something
		// blocking the first square, it's also blocking the second square.
		Bitboard moveBlockers = board->allPieces & ~BB_SET[mover];
		if(board->turn == WHITE) {
			moveBlockers |= (moveBlockers & 0x0000000000FF0000L) << 8;
		} else {
			moveBlockers |= (moveBlockers & 0x0000FF0000000000L) >> 8;
		}
		Bitboard attackSquaresMove =
			attackSquaresPawnMove[board->turn][mover] & ~moveBlockers;
		Bitboard attackSquaresCapture =
			attackSquaresPawnCapture[board->turn][mover];
		attackSquaresCapture &= (board->playerBitboards[FLIP(board->turn)] |
			board->enPassantTarget);
		while(attackSquaresMove) {
			int attackSquare = popBit(&attackSquaresMove);
			Move move = MOVE(mover, attackSquare);
			if(isPromotable) {
				*moves++ = move | MOVE_PROMOTION | MOVE_PROMOTE_TO_QUEEN;
				*moves++ = move | MOVE_PROMOTION | MOVE_PROMOTE_TO_KNIGHT;
				*moves++ = move | MOVE_PROMOTION | MOVE_PROMOTE_TO_ROOK;
				*moves++ = move | MOVE_PROMOTION | MOVE_PROMOTE_TO_BISHOP;
			} else {
				*moves++ = move;
			}
		}
		while(attackSquaresCapture) {
			int attackSquare = popBit(&attackSquaresCapture);
			Move move = MOVE(mover, attackSquare);
			if(BB_SET[attackSquare] & board->enPassantTarget) {
				move |= MOVE_ENPASSANT;
			}
			if(isPromotable) {
				*moves++ = move | MOVE_PROMOTION | MOVE_PROMOTE_TO_QUEEN;
				*moves++ = move | MOVE_PROMOTION | MOVE_PROMOTE_TO_KNIGHT;
				*moves++ = move | MOVE_PROMOTION | MOVE_PROMOTE_TO_ROOK;
				*moves++ = move | MOVE_PROMOTION | MOVE_PROMOTE_TO_BISHOP;
			} else {
				*moves++ = move;
			}
		}
	}
	return moves;
}

Move *MoveGen::appendMovesForShortRangePiece(Bitboard bitboard,
		Bitboard *attackSquares, Board *board, Move *moves) {
	while(bitboard) {
		int mover = popBit(&bitboard);
		Bitboard attackSquaresFiltered = attackSquares[mover] &
			~board->playerBitboards[board->turn];
		while(attackSquaresFiltered) {
			int attackSquare = popBit(&attackSquaresFiltered);
			*moves++ = MOVE(mover, attackSquare);
		}
	}
	return moves;
}

Move *MoveGen::appendMovesForLongRangePiece(Bitboard bitboard,
		PieceType movementType, Board *board, Move *moves) {
	while(bitboard) {
		int mover = popBit(&bitboard);
		Bitboard attackSquares = slidingAttack.getAttackSquares(mover,
			movementType, board->allPieces, board->playerBitboards[board->turn]);
		while(attackSquares) {
			int attackSquare = popBit(&attackSquares);
			*moves++ = MOVE(mover, attackSquare);
		}
	}
	return moves;
}

Move *MoveGen::appendMovesForCastling(Board *board, Move *moves) {
	for(int castleType = KINGSIDE; castleType <= QUEENSIDE; castleType++) {
		CastleRight right = (CastleRight) CASTLE_RIGHT(board->turn, castleType);
		if(!(board->hasCastleRight(right))) {
			continue;
		}
		if(board->allPieces & castleBlockers[right]) {
			continue;
		}
		bool inCheck = false;
		for(int i = 0; i < 3; i++) {
			Square castleCheckSquare = (Square) castlePath[right][i];
			if(isSquareAttacked(board, castleCheckSquare)) {
				inCheck = true;
				break;
			}
		}
		if(inCheck) {
			continue;
		}
		Move move = MOVE(castlePath[right][0], castlePath[right][2]);
		move |= MOVE_CASTLE;
		*moves++ = move;
	}
	return moves;
}

Move *MoveGen::legalMovesFast(Board *board, Move *moves) {
	Move *movesCurrent = moves;
	movesCurrent = appendMovesForPawn(board, movesCurrent);
	movesCurrent = appendMovesForShortRangePiece(
		board->bitboards[PIECE(board->turn, KNIGHT)], attackSquaresKnight,
		board, movesCurrent);
	movesCurrent = appendMovesForLongRangePiece(
		board->bitboards[PIECE(board->turn, BISHOP)], BISHOP, board,
		movesCurrent);
	movesCurrent = appendMovesForLongRangePiece(
		board->bitboards[PIECE(board->turn, ROOK)], ROOK, board,
		movesCurrent);
	movesCurrent = appendMovesForLongRangePiece(
		board->bitboards[PIECE(board->turn, QUEEN)], BISHOP, board,
		movesCurrent);
	movesCurrent = appendMovesForLongRangePiece(
		board->bitboards[PIECE(board->turn, QUEEN)], ROOK, board,
		movesCurrent);
	movesCurrent = appendMovesForShortRangePiece(
		board->bitboards[PIECE(board->turn, KING)], attackSquaresKing,
		board, movesCurrent);
	movesCurrent = appendMovesForCastling(board, movesCurrent);
	return movesCurrent;
}

bool MoveGen::isSquareAttacked(Board *board, Square square) {
	Bitboard bitboard;
	Bitboard bbSquare = BB_SET[square];
	int turnFlipped = FLIP(board->turn);

	bitboard = board->bitboards[PIECE(turnFlipped, PAWN)];
	while(bitboard) {
		int mover = popBit(&bitboard);
		Bitboard attackSquares = attackSquaresPawnCapture[turnFlipped][mover];
		if(attackSquares & bbSquare) {
			return true;
		}
	}

	bitboard = board->bitboards[PIECE(turnFlipped, KNIGHT)];
	while(bitboard) {
		int mover = popBit(&bitboard);
		Bitboard attackSquares = attackSquaresKnight[mover];
		if(attackSquares & bbSquare) {
			return true;
		}
	}

	bitboard = board->bitboards[PIECE(turnFlipped, KING)];
	while(bitboard) {
		int mover = popBit(&bitboard);
		Bitboard attackSquares = attackSquaresKing[mover];
		if(attackSquares & bbSquare) {
			return true;
		}
	}

	bitboard = board->bitboards[PIECE(turnFlipped, BISHOP)];
	while(bitboard) {
		int mover = popBit(&bitboard);
		Bitboard attackSquares = slidingAttack.getAttackSquares(mover,
			BISHOP, board->allPieces, board->playerBitboards[turnFlipped]);
		if(attackSquares & bbSquare) {
			return true;
		}
	}

	bitboard = board->bitboards[PIECE(turnFlipped, ROOK)];
	while(bitboard) {
		int mover = popBit(&bitboard);
		Bitboard attackSquares = slidingAttack.getAttackSquares(mover,
			ROOK, board->allPieces, board->playerBitboards[turnFlipped]);
		if(attackSquares & bbSquare) {
			return true;
		}
	}

	bitboard = board->bitboards[PIECE(turnFlipped, QUEEN)];
	while(bitboard) {
		int mover = popBit(&bitboard);
		Bitboard attackSquaresDiagonal = slidingAttack.getAttackSquares(mover,
			BISHOP, board->allPieces, board->playerBitboards[turnFlipped]);
		if(attackSquaresDiagonal & bbSquare) {
			return true;
		}
		Bitboard attackSquaresStraight = slidingAttack.getAttackSquares(mover,
			ROOK, board->allPieces, board->playerBitboards[turnFlipped]);
		if(attackSquaresStraight & bbSquare) {
			return true;
		}
	}
	return false;
}

bool MoveGen::isLegal(Board *board, Move move) {
	bool isAttacked;
	board->move(move);
	board->turn = FLIP(board->turn);
	Bitboard ourKings = board->bitboards[PIECE(board->turn, KING)];
	Square ourKingSquare = (Square) popBit(&ourKings);
	isAttacked = isSquareAttacked(board, ourKingSquare);
	board->turn = FLIP(board->turn);
	board->unmove(move);
	return !isAttacked;
}
