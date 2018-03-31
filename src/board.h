#ifndef SRC_BOARD_H_
#define SRC_BOARD_H_

#include <string>

#include "types.h"

struct Undo {
	Piece capturedPiece;
	Piece movedPiece;
	Bitboard enPassantTarget;
	unsigned int castleRights;
};

class Board {
public:
	Board();
	void print();
	void move(Move move);
	void unmove(Move move);
	void setPosition(std::string fen);
	bool hasCastleRight(CastleRight castleRight);

	Bitboard bitboards[PIECE_NONE];
	Bitboard playerBitboards[2];
	Bitboard allPieces;
	Bitboard enPassantTarget;
	Color turn;
	// Bits 0-4 are used according to the enum CastleRight
	unsigned int castleRights;
	int ply;
private:
	void clear();
	void updateSummaryBitboards();
	void setEnPassantTarget(Move move);
	void unsetEnPassantTarget();
	void moveEnPassant(Move move);
	void unmoveEnPassant(Move move);
	void removeCastleRights();
	void updateCastleRightsRookMove(Bitboard occSource);
	void updateCastleRightsRookCapture(Bitboard occDest);
	void moveCastle(Move move);
	void unmoveCastle(Move move);
	void movePromote(Move move);
	void unmovePromote(Move move);
	inline void setCastleRight(CastleRight castleRight);
	inline void clearCastleRight(CastleRight castleRight);

	Undo undoHistory[MAX_PLIES];
};

#endif /* SRC_BOARD_H_ */
