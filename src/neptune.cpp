#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <poll.h>
#include <sys/time.h>
#include <unistd.h>

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
		return "(none)";
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
	if(promotion >= 'A' && promotion <= 'Z') {
		promotion = (char) (promotion - 'A' + 'a');
	}
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

std::string inputBuffer;
bool inputEof = false;
bool inputBlocked = false;
bool quitRequested = false;
bool openEndedSearch = false;
Brain *activeBrain = nullptr;
Board *activeBoard = nullptr;

bool takeLine(std::string *line) {
	std::string::size_type pos = inputBuffer.find('\n');
	if(pos == std::string::npos) {
		return false;
	}
	*line = inputBuffer.substr(0, pos);
	if(!line->empty() && (*line)[line->size() - 1] == '\r') {
		line->pop_back();
	}
	inputBuffer.erase(0, pos + 1);
	return true;
}

bool pullInput() {
	struct pollfd pfd;
	pfd.fd = STDIN_FILENO;
	pfd.events = POLLIN;
	int ready = poll(&pfd, 1, 0);
	if(ready <= 0) {
		return false;
	}
	if(pfd.revents & POLLIN) {
		char tmp[4096];
		ssize_t n = ::read(STDIN_FILENO, tmp, sizeof tmp);
		if(n < 0) {
			return false;
		}
		if(n == 0) {
			inputEof = true;
			return false;
		}
		inputBuffer.append(tmp, tmp + n);
		return true;
	}
	if(pfd.revents & POLLHUP) {
		inputEof = true;
	}
	return false;
}

bool readLine(std::string &line) {
	while(!takeLine(&line)) {
		if(inputEof) {
			if(inputBuffer.empty()) {
				return false;
			}
			line = inputBuffer;
			inputBuffer.clear();
			return true;
		}
		char tmp[4096];
		ssize_t n = ::read(STDIN_FILENO, tmp, sizeof tmp);
		if(n < 0) {
			if(errno == EINTR) {
				continue;
			}
			return false;
		}
		if(n == 0) {
			inputEof = true;
			continue;
		}
		inputBuffer.append(tmp, tmp + n);
	}
	inputBlocked = false;
	return true;
}

// stop ends the search. quit ends an open-ended search. Anything else stays
// buffered for the main loop, including quit after a depth or a clock.
bool pollSearchInput() {
	if(inputBlocked) {
		return false;
	}
	while(true) {
		std::string line;
		if(!takeLine(&line)) {
			if(inputEof) {
				quitRequested = true;
				return true;
			}
			if(!pullInput()) {
				if(inputEof && inputBuffer.empty()) {
					quitRequested = true;
					return true;
				}
				return false;
			}
			continue;
		}
		std::istringstream ss(line);
		std::string token;
		if(!(ss >> token)) {
			continue;
		}
		if(token == "isready") {
			std::cout << "readyok" << std::endl;
			continue;
		}
		if(token == "stop") {
			return true;
		}
		if(token == "quit" && openEndedSearch) {
			quitRequested = true;
			return true;
		}
		inputBuffer.insert(0, line + "\n");
		inputBlocked = true;
		return false;
	}
}

void reportInfo(SearchResult result, long long nodes, int elapsedMs) {
	Move variation[64];
	int variationLength = activeBrain->principalVariation(activeBoard, result.move, variation,
		result.depth > 0 ? result.depth : 1);
	std::cout << "info depth " << result.depth << " ";
	if(Brain::isMateScore(result.score)) {
		std::cout << "score mate " << Brain::mateMoves(result.score);
	} else {
		std::cout << "score cp " << result.score;
	}
	std::cout << " nodes " << nodes << " time " << elapsedMs << " pv";
	for(int i = 0; i < variationLength; i++) {
		std::cout << " " << moveToUci(variation[i]);
	}
	std::cout << std::endl;
}

SearchLimits parseGo(std::istringstream *ss, Color turn) {
	bool depthSet = false;
	int depth = 8;
	bool infinite = false;
	bool movetimeSet = false;
	int movetime = 0;
	bool wtimeSet = false;
	bool btimeSet = false;
	int wtime = 0;
	int btime = 0;
	int winc = 0;
	int binc = 0;
	bool movestogoSet = false;
	int movestogo = 0;
	std::string token;
	while(*ss >> token) {
		if(token == "depth") {
			*ss >> depth;
			depthSet = true;
		} else if(token == "infinite") {
			infinite = true;
		} else if(token == "movetime") {
			*ss >> movetime;
			movetimeSet = true;
		} else if(token == "wtime") {
			*ss >> wtime;
			wtimeSet = true;
		} else if(token == "btime") {
			*ss >> btime;
			btimeSet = true;
		} else if(token == "winc") {
			*ss >> winc;
		} else if(token == "binc") {
			*ss >> binc;
		} else if(token == "movestogo") {
			*ss >> movestogo;
			movestogoSet = true;
		}
	}
	SearchLimits limits;
	limits.useTime = false;
	limits.softMs = 0;
	limits.hardMs = 0;
	limits.untilStop = false;
	if(movetimeSet) {
		limits.useTime = true;
		limits.softMs = movetime;
		limits.hardMs = movetime;
	} else {
		bool white = turn == WHITE;
		bool hasClock = white ? wtimeSet : btimeSet;
		if(hasClock) {
			int remaining = white ? wtime : btime;
			int inc = white ? winc : binc;
			int moves = (movestogoSet && movestogo > 0) ? movestogo : 20;
			limits.useTime = true;
			if(remaining <= 0) {
				limits.softMs = 0;
				limits.hardMs = 0;
			} else {
				int usable = remaining > 50 ? remaining - 50 : remaining;
				long long soft = (long long) remaining / moves + inc;
				if(soft > usable) {
					soft = usable;
				}
				long long hard = soft * 2;
				if(hard > usable) {
					hard = usable;
				}
				limits.softMs = (int) soft;
				limits.hardMs = (int) hard;
			}
		}
	}
	if(depthSet) {
		limits.maxDepth = depth;
	} else if(infinite || limits.useTime) {
		limits.maxDepth = Brain::MAX_DEPTH - 1;
	} else {
		limits.maxDepth = 8;
	}
	// A depth or a clock ends on its own. quit then belongs to the main loop.
	// An open search would otherwise run to the depth cap.
	limits.untilStop = infinite && !depthSet && !limits.useTime;
	return limits;
}

const int HASH_DEFAULT = 64;
const int HASH_MIN = 1;
const int HASH_MAX = 1024;

static bool sameOption(const std::string &left, const std::string &right) {
	if(left.size() != right.size()) {
		return false;
	}
	for(std::string::size_type i = 0; i < left.size(); i++) {
		unsigned char a = (unsigned char) left[i];
		unsigned char b = (unsigned char) right[i];
		if(std::tolower(a) != std::tolower(b)) {
			return false;
		}
	}
	return true;
}

static std::string trim(const std::string &text) {
	std::string::size_type start = 0;
	while(start < text.size() && std::isspace((unsigned char) text[start])) {
		start++;
	}
	std::string::size_type end = text.size();
	while(end > start && std::isspace((unsigned char) text[end - 1])) {
		end--;
	}
	return text.substr(start, end - start);
}

// The whole token has to be an integer. "16abc" is not a spin value.
static bool parseSpin(const std::string &text, int *out) {
	if(text.empty()) {
		return false;
	}
	errno = 0;
	char *end = nullptr;
	long parsed = std::strtol(text.c_str(), &end, 10);
	if(end == text.c_str() || *end != '\0' || errno == ERANGE) {
		return false;
	}
	if(parsed > INT_MAX || parsed < INT_MIN) {
		return false;
	}
	*out = (int) parsed;
	return true;
}

static int clampHashMegabytes(int megabytes) {
	if(megabytes < HASH_MIN) {
		return HASH_MIN;
	}
	if(megabytes > HASH_MAX) {
		return HASH_MAX;
	}
	return megabytes;
}

// Applied while the engine is waiting. A setoption during go stays buffered
// until bestmove, which is where the main loop reads it.
static void applySetOption(std::istringstream *ss, Brain *brain, int *hashMb, int *threads,
		std::string *evalFile) {
	std::string token;
	if(!(*ss >> token) || token != "name") {
		return;
	}
	std::string name;
	bool sawValue = false;
	while(*ss >> token) {
		if(token == "value") {
			sawValue = true;
			break;
		}
		if(!name.empty()) {
			name += ' ';
		}
		name += token;
	}
	if(name.empty()) {
		return;
	}
	std::string value;
	if(sawValue) {
		std::string rest;
		std::getline(*ss, rest);
		value = trim(rest);
	}
	if(sameOption(name, "Hash")) {
		int megabytes = 0;
		if(!parseSpin(value, &megabytes)) {
			return;
		}
		*hashMb = clampHashMegabytes(megabytes);
		brain->setHashMegabytes(*hashMb);
	} else if(sameOption(name, "Threads")) {
		int requested = 0;
		if(!parseSpin(value, &requested)) {
			return;
		}
		// One thread owns the board. Any integer is accepted as that one thread.
		if(requested < 1 || requested > 1) {
			requested = 1;
		}
		*threads = requested;
	} else if(sameOption(name, "EvalFile")) {
		if(value.empty() || value == "<empty>") {
			evalFile->clear();
		} else {
			*evalFile = value;
		}
	}
}

static void printOption(const std::string &name, int hashMb, int threads, const std::string &evalFile) {
	if(sameOption(name, "Hash")) {
		std::cout << hashMb << std::endl;
	} else if(sameOption(name, "Threads")) {
		std::cout << threads << std::endl;
	} else if(sameOption(name, "EvalFile")) {
		std::cout << (evalFile.empty() ? "<empty>" : evalFile) << std::endl;
	}
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
	int hashMb = HASH_DEFAULT;
	int threads = 1;
	std::string evalFile;
	std::string line, token;

	initBitboards();

	while(readLine(line)) {
		std::istringstream ss(line);
		if(!(ss >> token)) {
			continue;
		}
		if(token == "uci") {
			std::cout << "id name Neptune" << std::endl;
			std::cout << "id author Phil Leszczynski" << std::endl;
			std::cout << "option name Hash type spin default " << HASH_DEFAULT
				<< " min " << HASH_MIN << " max " << HASH_MAX << std::endl;
			std::cout << "option name Threads type spin default 1 min 1 max 1" << std::endl;
			std::cout << "option name EvalFile type string default <empty>" << std::endl;
			std::cout << "uciok" << std::endl;
		} else if(token == "setoption") {
			applySetOption(&ss, &brain, &hashMb, &threads, &evalFile);
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
		} else if(token == "_tt") {
			std::cout << brain.transpositionEntries() << std::endl;
		} else if(token == "_option") {
			if(ss >> token) {
				printOption(token, hashMb, threads, evalFile);
			}
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
			SearchLimits limits = parseGo(&ss, board.turn);
			activeBrain = &brain;
			activeBoard = &board;
			quitRequested = false;
			openEndedSearch = limits.untilStop;
			SearchResult result = brain.getMove(&board, limits, pollSearchInput, reportInfo);
			activeBrain = nullptr;
			activeBoard = nullptr;
			if(quitRequested) {
				return 0;
			}
			std::cout << "bestmove " << moveToUci(result.move) << std::endl;
		} else if(token == "quit") {
			return 0;
		}
	}
	return 0;
}
