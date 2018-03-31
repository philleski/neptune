#include <stdio.h>
#include <sstream>

#include "bitboard.h"
#include "board.h"

Board::Board() {
	int piece;
	int square;

	initBitboards();

	for(piece = 0; piece < PIECE_NONE; piece++) {
		bitboards[piece] = BB_EMPTY;
	}
	for(square = A2; square <= H2; square++) {
		bitboards[WHITE_PAWN] |= BB_SET[square];
	}
	for(square = A7; square <= H7; square++) {
		bitboards[BLACK_PAWN] |= BB_SET[square];
	}
	bitboards[WHITE_KNIGHT] |= BB_SET[B1];
	bitboards[WHITE_KNIGHT] |= BB_SET[G1];
	bitboards[BLACK_KNIGHT] |= BB_SET[B8];
	bitboards[BLACK_KNIGHT] |= BB_SET[G8];
	bitboards[WHITE_BISHOP] |= BB_SET[C1];
	bitboards[WHITE_BISHOP] |= BB_SET[F1];
	bitboards[BLACK_BISHOP] |= BB_SET[C8];
	bitboards[BLACK_BISHOP] |= BB_SET[F8];
	bitboards[WHITE_ROOK] |= BB_SET[A1];
	bitboards[WHITE_ROOK] |= BB_SET[H1];
	bitboards[BLACK_ROOK] |= BB_SET[A8];
	bitboards[BLACK_ROOK] |= BB_SET[H8];
	bitboards[WHITE_QUEEN] |= BB_SET[D1];
	bitboards[BLACK_QUEEN] |= BB_SET[D8];
	bitboards[WHITE_KING] |= BB_SET[E1];
	bitboards[BLACK_KING] |= BB_SET[E8];

	updateSummaryBitboards();

	setCastleRight(WHITE_KINGSIDE);
	setCastleRight(WHITE_QUEENSIDE);
	setCastleRight(BLACK_KINGSIDE);
	setCastleRight(BLACK_QUEENSIDE);

	enPassantTarget = BB_EMPTY;
	turn = WHITE;
	ply = 0;
}

void Board::print() {
	for(int rank = RANK_8; rank >= RANK_1; rank--) {
		for(int file = FILE_A; file <= FILE_H; file++) {
			Bitboard occupancy = BB_SET[SQ(rank, file)];
			if(bitboards[WHITE_PAWN] & occupancy) {
				printf("P");
			} else if(bitboards[BLACK_PAWN] & occupancy) {
				printf("p");
			} else if(bitboards[WHITE_KNIGHT] & occupancy) {
				printf("N");
			} else if(bitboards[BLACK_KNIGHT] & occupancy) {
				printf("n");
			} else if(bitboards[WHITE_BISHOP] & occupancy) {
				printf("B");
			} else if(bitboards[BLACK_BISHOP] & occupancy) {
				printf("b");
			} else if(bitboards[WHITE_ROOK] & occupancy) {
				printf("R");
			} else if(bitboards[BLACK_ROOK] & occupancy) {
				printf("r");
			} else if(bitboards[WHITE_QUEEN] & occupancy) {
				printf("Q");
			} else if(bitboards[BLACK_QUEEN] & occupancy) {
				printf("q");
			} else if(bitboards[WHITE_KING] & occupancy) {
				printf("K");
			} else if(bitboards[BLACK_KING] & occupancy) {
				printf("k");
			} else {
				printf("-");
			}
		}
		printf("\n");
	}
	std::string castle = "";
	if(hasCastleRight(WHITE_KINGSIDE)) {
		castle += "K";
	}
	if(hasCastleRight(WHITE_QUEENSIDE)) {
		castle += "Q";
	}
	if(hasCastleRight(BLACK_KINGSIDE)) {
		castle += "k";
	}
	if(hasCastleRight(BLACK_QUEENSIDE)) {
		castle += "q";
	}
	if(castle.empty()) {
		castle = "-";
	}
	printf("Castle: %s\n", castle.c_str());
}

void Board::updateSummaryBitboards() {
	playerBitboards[WHITE] = bitboards[WHITE_PAWN] | bitboards[WHITE_KNIGHT] |
		bitboards[WHITE_BISHOP] | bitboards[WHITE_ROOK] |
		bitboards[WHITE_QUEEN] | bitboards[WHITE_KING];
	playerBitboards[BLACK] = bitboards[BLACK_PAWN] | bitboards[BLACK_KNIGHT] |
		bitboards[BLACK_BISHOP] | bitboards[BLACK_ROOK] |
		bitboards[BLACK_QUEEN] | bitboards[BLACK_KING];
	allPieces = playerBitboards[WHITE] | playerBitboards[BLACK];
}

void Board::clear() {
	for(int piece = WHITE_PAWN; piece <= BLACK_KING; piece++) {
		bitboards[piece] = BB_EMPTY;
	}
	ply = 0;
	enPassantTarget = BB_EMPTY;
	turn = WHITE;
	clearCastleRight(WHITE_KINGSIDE);
	clearCastleRight(WHITE_QUEENSIDE);
	clearCastleRight(BLACK_KINGSIDE);
	clearCastleRight(BLACK_QUEENSIDE);
	updateSummaryBitboards();
}

// Based off https://github.com/official-stockfish/Stockfish/blob/master/src/position.cpp
void Board::setPosition(std::string fen) {
	unsigned char column, row, token;
	int halfmove, fullmove, square = A8;
	std::istringstream ss(fen);

	ss >> std::noskipws;
	clear();

	// Step 1: Piece placement
	while((ss >> token) && !isspace(token)) {
		if(isdigit(token)) {
			square += (token - '0');
		} else if(token == '/') {
			square -= 16;
		} else {
			switch(token) {
			case 'P':
				bitboards[WHITE_PAWN] |= BB_SET[square];
				break;
			case 'N':
				bitboards[WHITE_KNIGHT] |= BB_SET[square];
				break;
			case 'B':
				bitboards[WHITE_BISHOP] |= BB_SET[square];
				break;
			case 'R':
				bitboards[WHITE_ROOK] |= BB_SET[square];
				break;
			case 'Q':
				bitboards[WHITE_QUEEN] |= BB_SET[square];
				break;
			case 'K':
				bitboards[WHITE_KING] |= BB_SET[square];
				break;
			case 'p':
				bitboards[BLACK_PAWN] |= BB_SET[square];
				break;
			case 'n':
				bitboards[BLACK_KNIGHT] |= BB_SET[square];
				break;
			case 'b':
				bitboards[BLACK_BISHOP] |= BB_SET[square];
				break;
			case 'r':
				bitboards[BLACK_ROOK] |= BB_SET[square];
				break;
			case 'q':
				bitboards[BLACK_QUEEN] |= BB_SET[square];
				break;
			case 'k':
				bitboards[BLACK_KING] |= BB_SET[square];
				break;
			}
			square++;
		}
	}
	updateSummaryBitboards();

	// Step 2: Turn to move
	ss >> token;
	turn = (token == 'w' ? WHITE : BLACK);
	ss >> token;

	// Step 3: Castling rights
	while((ss >> token) && !isspace(token)) {
		if(token == 'K') {
			setCastleRight(WHITE_KINGSIDE);
		} else if(token == 'Q') {
			setCastleRight(WHITE_QUEENSIDE);
		} else if(token == 'k') {
			setCastleRight(BLACK_KINGSIDE);
		} else if(token == 'q') {
			setCastleRight(BLACK_QUEENSIDE);
		}
	}

	// Step 4: En passant square
	ss >> column;
	if(column >= 'a' && column <= 'h') {
		ss >> row;
		if(row == '3' || row == '6') {
			int target = A1 + (column - 'a') + 8 * (row - '1');
			enPassantTarget = BB_SET[target];
		}
	}

	// Steps 5-6: Halfmove clock and Ply Count
	ss >> std::skipws >> halfmove >> fullmove;
	ply = 2 * (fullmove - 1) + (turn == BLACK);
}

void Board::setEnPassantTarget(Move move) {
	unsigned int target = SOURCE(move) + FORWARD(turn);
	enPassantTarget = BB_SET[target];
}

void Board::unsetEnPassantTarget() {
	enPassantTarget = BB_EMPTY;
}

void Board::moveEnPassant(Move move) {
	Color turnFlipped = FLIP(turn);
	unsigned int target = DEST(move) + BACKWARD(turn);
	bitboards[PIECE(turnFlipped, PAWN)] &= BB_CLR[target];
	playerBitboards[turnFlipped] &= BB_CLR[target];
	allPieces &= BB_CLR[target];
}

void Board::unmoveEnPassant(Move move) {
	Color turnFlipped = FLIP(turn);
	unsigned int target = DEST(move) + BACKWARD(turn);
	bitboards[PIECE(turnFlipped, PAWN)] |= BB_SET[target];
	playerBitboards[turnFlipped] |= BB_SET[target];
	allPieces |= BB_SET[target];
}

void Board::removeCastleRights() {
	clearCastleRight((CastleRight) CASTLE_RIGHT(this->turn, KINGSIDE));
	clearCastleRight((CastleRight) CASTLE_RIGHT(this->turn, QUEENSIDE));
}

void Board::updateCastleRightsRookMove(Bitboard occSource) {
	if(BB_FILE[FILE_A] & occSource) {
		clearCastleRight((CastleRight) CASTLE_RIGHT(this->turn, QUEENSIDE));
	} else if(BB_FILE[FILE_H] & occSource) {
		clearCastleRight((CastleRight) CASTLE_RIGHT(this->turn, KINGSIDE));
	}
}

void Board::updateCastleRightsRookCapture(Bitboard occDest) {
	// When capturing an opposing rook, remove the opponent's right to castle
	// in that direction.
	Color turnFlipped = FLIP(this->turn);
	if(BB_FILE[FILE_A] & occDest) {
		clearCastleRight((CastleRight) CASTLE_RIGHT(turnFlipped, QUEENSIDE));
	} else if(BB_FILE[FILE_H] & occDest) {
		clearCastleRight((CastleRight) CASTLE_RIGHT(turnFlipped, KINGSIDE));
	}
}

void Board::moveCastle(Move move) {
	// The king has already moved, so just move the rook.
	int source = SOURCE(move);
	int destination = DEST(move);
	CastleType castleType;
	if(source - destination == 2) {
		castleType = QUEENSIDE;
	} else if(source - destination == -2) {
		castleType = KINGSIDE;
	} else {
		return;
	}
	Bitboard occRookStartClr = BB_CLR[CASTLE_ROOK_START(turn, castleType)];
	Bitboard occRookEnd = BB_SET[CASTLE_ROOK_END(turn, castleType)];
	bitboards[PIECE(turn, ROOK)] &= occRookStartClr;
	bitboards[PIECE(turn, ROOK)] |= occRookEnd;
	playerBitboards[turn] &= occRookStartClr;
	playerBitboards[turn] |= occRookEnd;
	allPieces &= occRookStartClr;
	allPieces |= occRookEnd;
}

void Board::unmoveCastle(Move move) {
	// The king has already moved, so just move the rook.
	int source = SOURCE(move);
	int destination = DEST(move);
	CastleType castleType;
	if(source - destination == 2) {
		castleType = QUEENSIDE;
	} else if(source - destination == -2) {
		castleType = KINGSIDE;
	} else {
		return;
	}
	Bitboard occRookStart = BB_SET[CASTLE_ROOK_START(turn, castleType)];
	Bitboard occRookEndClr = BB_CLR[CASTLE_ROOK_END(turn, castleType)];
	bitboards[PIECE(turn, ROOK)] |= occRookStart;
	bitboards[PIECE(turn, ROOK)] &= occRookEndClr;
	playerBitboards[turn] |= occRookStart;
	playerBitboards[turn] &= occRookEndClr;
	allPieces |= occRookStart;
	allPieces &= occRookEndClr;
}

void Board::movePromote(Move move) {
	int destination = DEST(move);
	Bitboard occDest = BB_SET[destination];
	// The pawn was already transferred to the destination square so remove it.
	bitboards[PIECE(turn, PAWN)] &= ~occDest;
	bitboards[PIECE(turn, PROMOTION_PIECE(move))] |= occDest;
}

void Board::unmovePromote(Move move) {
	int destination = DEST(move);
	Bitboard occDestClr = BB_CLR[destination];
	// The pawn will be removed but no need to add it back in.
	bitboards[PIECE(turn, PROMOTION_PIECE(move))] &= occDestClr;
}

void Board::move(Move move) {
	undoHistory[ply].castleRights = castleRights;
	undoHistory[ply].enPassantTarget = enPassantTarget;
	Bitboard occSource = BB_SET[SOURCE(move)];
	Bitboard occDest = BB_SET[DEST(move)];
	int pieceStartUs = turn == WHITE ? WHITE_PAWN : BLACK_PAWN;
	int pieceEndUs = turn == WHITE ? WHITE_KING : BLACK_KING;
	int turnFlipped = FLIP(turn);
	for(int piece = pieceStartUs; piece <= pieceEndUs; piece++) {
		if(!(bitboards[piece] & occSource)) {
			continue;
		}
		if(occDest & bitboards[PIECE(turnFlipped, ROOK)]) {
			if(occDest & BB_ROOK_START[turnFlipped]) {
				updateCastleRightsRookCapture(occDest);
			}
		}
		bitboards[piece] &= ~occSource;
		bitboards[piece] |= occDest;
		playerBitboards[turn] &= ~occSource;
		playerBitboards[turn] |= occDest;
		allPieces &= ~occSource;
		allPieces |= occDest;
		if(IS_ENPASSANT(move)) {
			moveEnPassant(move);
		}
		if((piece == WHITE_PAWN || piece == BLACK_PAWN) &&
				(occSource << 16 == occDest || occSource >> 16 == occDest)) {
			// Pawn moving forward two squares.
			setEnPassantTarget(move);
		} else {
			unsetEnPassantTarget();
		}
		if(occDest & playerBitboards[turnFlipped]) {
			int pieceStartOpp = turn == WHITE ? BLACK_PAWN : WHITE_PAWN;
			int pieceEndOpp = turn == WHITE ? BLACK_KING : WHITE_KING;
			for(int capturedPiece = pieceStartOpp; capturedPiece <= pieceEndOpp; capturedPiece++) {
				if(!(bitboards[capturedPiece] & occDest)) {
					continue;
				}
				bitboards[capturedPiece] &= ~occDest;
				playerBitboards[turnFlipped] &= ~occDest;
				undoHistory[ply].capturedPiece = (Piece) capturedPiece;
				break;
			}
		} else {
			undoHistory[ply].capturedPiece = PIECE_NONE;
		}
		if(piece == WHITE_KING || piece == BLACK_KING) {
			removeCastleRights();
			if(IS_CASTLING(move)) {
				moveCastle(move);
			}
		} else if(piece == WHITE_ROOK || piece == BLACK_ROOK) {
			updateCastleRightsRookMove(occSource);
		} else if(IS_PROMOTION(move)) {
			movePromote(move);
		}
		undoHistory[ply].movedPiece = (Piece) piece;
		break;
	}
	turn = (Color) turnFlipped;
	ply++;
}

void Board::unmove(Move move) {
	Bitboard occSource = BB_SET[SOURCE(move)];
	Bitboard occDest = BB_SET[DEST(move)];
	int turnFlipped = turn;
	turn = FLIP(turn);
	ply--;
	castleRights = undoHistory[ply].castleRights;
	enPassantTarget = undoHistory[ply].enPassantTarget;

	int piece = undoHistory[ply].movedPiece;
	int capturedPiece = undoHistory[ply].capturedPiece;
	bitboards[piece] &= ~occDest;
	bitboards[piece] |= occSource;
	playerBitboards[turn] &= ~occDest;
	playerBitboards[turn] |= occSource;
	allPieces |= occSource;
	if(capturedPiece == PIECE_NONE) {
		allPieces &= ~occDest;
	} else {
		bitboards[capturedPiece] |= occDest;
		playerBitboards[turnFlipped] |= occDest;
	}
	if(IS_ENPASSANT(move)) {
		unmoveEnPassant(move);
	} else if(IS_CASTLING(move)) {
		unmoveCastle(move);
	} else if(IS_PROMOTION(move)) {
		unmovePromote(move);
	}
}

void Board::setCastleRight(CastleRight castleRight) {
	castleRights |= (1 << castleRight);
}

void Board::clearCastleRight(CastleRight castleRight) {
	castleRights &= ~(1 << castleRight);
}

bool Board::hasCastleRight(CastleRight castleRight) {
	return castleRights & (1 << castleRight);
}

