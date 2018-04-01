#ifndef SRC_TYPES_H_
#define SRC_TYPES_H_

#define MAX_MOVES 200
#define MAX_PLIES 2000

typedef unsigned long long U64;

typedef U64 Bitboard;
const Bitboard BB_EMPTY = 0ULL;

// Based off Stockfish code: https://github.com/official-stockfish/Stockfish/blob/master/src/types.h
// Bits 0-5: source square (0-63)
// Bits 6-11: destination square (0-63)
// Bits 12-13: promotion piece (0=KNIGHT, 1=BISHOP, 2=ROOK, 3=QUEEN)
// Bits 14-15: special move flag (0=none, 1=promotion, 2=en passant, 3=castling)
typedef unsigned int Move;
const Move MOVE_NONE = 0;
const Move MOVE_PROMOTION = 0x4000;
const Move MOVE_ENPASSANT = 0x8000;
const Move MOVE_CASTLE = 0xc000;
const Move MOVE_PROMOTE_TO_KNIGHT = 0x0000;
const Move MOVE_PROMOTE_TO_BISHOP = 0x1000;
const Move MOVE_PROMOTE_TO_ROOK = 0x2000;
const Move MOVE_PROMOTE_TO_QUEEN = 0x3000;
#define IS_PROMOTION(move) ((move & 0xc000) == 0x4000)
#define IS_ENPASSANT(move) ((move & 0xc000) == 0x8000)
#define IS_CASTLING(move) ((move & 0xc000) == 0xc000)
#define PROMOTION_PIECE(move) (((move & 0x3000) >> 12) + 1)
#define SOURCE(move) (move & 0x3f)
#define DEST(move) ((move & 0xfff) >> 6)

enum PieceType {
	PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, PIECE_TYPE_NONE
};

enum Piece {
	WHITE_PAWN, WHITE_KNIGHT, WHITE_BISHOP, WHITE_ROOK, WHITE_QUEEN,
	WHITE_KING, BLACK_PAWN, BLACK_KNIGHT, BLACK_BISHOP, BLACK_ROOK,
	BLACK_QUEEN, BLACK_KING, PIECE_NONE
};
#define PIECE(color, pieceType) (color * 6 + pieceType)

enum Color {
	WHITE, BLACK, COLOR_NONE
};
#define FLIP(color) (color == WHITE ? BLACK : WHITE)

enum CastleType {
	KINGSIDE, QUEENSIDE, CASTLE_TYPE_NONE
};

enum CastleRight {
	WHITE_KINGSIDE, WHITE_QUEENSIDE, BLACK_KINGSIDE, BLACK_QUEENSIDE,
	CASTLE_RIGHT_NONE
};
#define CASTLE_RIGHT(color, castleType) (color * 2 + castleType)
#define CASTLE_ROOK_START(color, castleType) ((castleType == QUEENSIDE ? A1 : H1) + (color == WHITE ? 0 : 56))
#define CASTLE_ROOK_END(color, castleType) ((castleType == QUEENSIDE ? D1 : F1) + (color == WHITE ? 0 : 56))

enum Rank {
	RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8
};

enum File {
	FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H
};

enum Square {
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8,
	SQUARE_NONE
};
#define MOVE(source, dest) (source + (dest << 6))

enum Direction {
	NORTH = 8, SOUTH = -8, WEST = -1, EAST = 1, NORTHWEST = 7, NORTHEAST = 9,
	SOUTHWEST = -9, SOUTHEAST = -7, DIRECTION_NONE = 0
};
#define BACKWARD(color) (color == WHITE ? SOUTH : NORTH)
#define FORWARD(color) (color == WHITE ? NORTH : SOUTH)
#define FORWARD2(color) (color == WHITE ? NORTH + NORTH : SOUTH + SOUTH)
#define FORWARD_WEST(color) (color == WHITE ? NORTHWEST : SOUTHWEST)
#define FORWARD_EAST(color) (color == WHITE ? NORTHEAST : SOUTHEAST)

#endif /* SRC_TYPES_H_ */
