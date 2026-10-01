CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O3

SRC_DIR := src
BUILD := build
DEBUG_BUILD := build/debug
DEBUG_CXXFLAGS := -std=c++17 -Wall -Wextra -O0 -g -fsanitize=address,undefined

SRCS := \
	bitboard.cpp \
	board.cpp \
	brain.cpp \
	evaluation.cpp \
	movegen.cpp \
	neptune.cpp \
	slidingattack.cpp \
	transposition.cpp \
	zobrist.cpp

OBJS := $(addprefix $(BUILD)/,$(SRCS:.cpp=.o))
DEPS := $(OBJS:.o=.d)

.PHONY: all clean test test-perft debug

all: $(BUILD)/neptune

$(BUILD)/neptune: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS)

$(BUILD)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) -MMD -MP -c -o $@ $<

-include $(DEPS)

debug:
	$(MAKE) all BUILD=$(DEBUG_BUILD) CXXFLAGS="$(DEBUG_CXXFLAGS)"

test: $(BUILD)/neptune
	python3 test/engine.py
	python3 test/perft.py --smoke

test-perft: $(BUILD)/neptune
	python3 test/perft.py

clean:
	rm -rf build
