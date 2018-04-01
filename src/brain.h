#ifndef SRC_BRAIN_H_
#define SRC_BRAIN_H_

#include "movegen.h"
#include "types.h"

class Brain {
public:
	Brain();
	Move getMove(Board *board);
private:
	MoveGen moveGen;
};

#endif /* SRC_BRAIN_H_ */
