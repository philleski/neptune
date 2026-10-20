# 0008. UCI options

Date: 2026-10-20

## Status

Accepted

## Context

A GUI cannot change Neptune after the handshake. `uci` prints the name and the author and then `uciok`. There is no `option` line, and the main loop has no `setoption` branch, so the command falls through and is discarded.

```371:374:src/neptune.cpp
		if(token == "uci") {
			std::cout << "id name Neptune" << std::endl;
			std::cout << "id author Phil Leszczynski" << std::endl;
			std::cout << "uciok" << std::endl;
```

The transposition table is allocated once, at a fixed power of two.

```27:29:src/transposition.h
	static const int SIZE_BITS = 22;
	static const int SIZE = 1 << SIZE_BITS;
	std::vector<TranspositionEntry> data;
```

`SIZE` is 4,194,304. `put` and `get` index with `hash & (SIZE - 1)`. `TranspositionEntry` is 16 bytes, so the table is 64 MiB. That matches the "about 64MB" table in [0001](0001-port-tactician-engine.md). `clear` zeroes those entries. Nothing reallocates them. The pawn/king table is a different store, 64K entries inside `Evaluation`, and it is not this size.

[0006](0006-time-control.md) gave `go` a clock. A clocked match wants two standard controls: `Hash`, the table size in megabytes, and `Threads`, the worker count. `Threads` has to be advertised even while the search stays on the one thread that owns the board. [0006](0006-time-control.md) left that thread alone, and this change does too.

A later network needs a third control, a path to a weight file. [0001](0001-port-tactician-engine.md) kept evaluation behind `Evaluation::fitness` so a loaded file could replace the hand-written score. There is still no file, and there is no option to name one. The option name used by engines that load a net is `EvalFile`. The parser has to accept `setoption` before `Hash`, `Threads`, or `EvalFile` can do anything. `scripts/elo.sh` already passes `option.Threads` to Stockfish. Neptune has nowhere to receive the same command.

`test/engine.py` reads the handshake as three exact lines: `id name`, `id author`, `uciok`. Option lines inserted before `uciok` fail that test until it expects them.

During `go`, `pollSearchInput` handles `isready`, `stop`, and `quit` on an open-ended search. Any other line, including `setoption`, is put back and stdin polling stops until the search returns. The main loop then sees that line. That is the right moment to apply an option. Applying `Hash` from inside the search would reallocate the table the search is reading.

## Decision

Accept `setoption` in the main loop, and answer `uci` with the three options below before `uciok`. The default table stays 4,194,304 entries, so a session that never sends `setoption` searches as it does now.

### Parser

A line is `setoption name <id> value <x>`. Tokens after `name` and before `value` are the id, joined by single spaces. The value is the rest of the line, with surrounding whitespace removed, so a path may contain spaces. A line with no `value` token has an empty value. The id match is case-insensitive. An unknown id is ignored and prints nothing. A spin value that is not an integer is ignored and leaves the previous setting.

The command is handled only in the main loop, while the engine is waiting. A `setoption` that arrives during `go` stays buffered, as [0006](0006-time-control.md) already buffers any other line, and it is applied after `bestmove`.

### Hash

`Hash` is a spin, default 64, minimum 1, maximum 1024. The unit is mebibytes, `1024 * 1024` bytes, which is what UCI means by megabytes.

The table stays a power of two, because the index is a mask. The entry count for a requested size is the largest power of two that fits in that many bytes:

- bytes = megabytes × 1024 × 1024
- slots = bytes / 16
- entries = the largest power of two less than or equal to slots

A value below 1 is clamped to 1. A value above 1024 is clamped to 1024. The sizes this produces:

| Megabytes | Entries |
| --- | --- |
| 1 | 65,536 |
| 3 | 131,072 |
| 16 | 1,048,576 |
| 64 | 4,194,304 |
| 1024 | 67,108,864 |

64 is exact, so the default allocation is unchanged. 3 rounds down to the 2 MiB table. The advertised value stays the clamped request: `Hash` 3 is still 3, and the entry count is 131,072.

`TranspositionTable` stores the entry count and masks with it. `resize` builds the new vector first and swaps it in only after that allocation succeeds, then zeroes it. A failed allocation leaves the old table. The same entry count does not allocate and does not clear. `ucinewgame` still clears whatever size is installed. The pawn/king table is not resized.

### Threads

`Threads` is a spin, default 1, minimum 1, maximum 1. Any integer is clamped to 1. The search, the board, and the stdin poll stay on the thread that called `getMove`. No worker is started. The option is advertised so a GUI can see that the engine is single-threaded and can send the usual `setoption` without it being an unknown name.

### EvalFile

`EvalFile` is a string, default `<empty>`. The empty value, and the UCI token `<empty>`, both store an empty path. Any other value is stored as the remainder of the line.

An empty path means `Evaluation::fitness` is the score, which is the hand-written eval. A stored path is not opened, and it is not read by search. `_eval` is unchanged by the option. The network change is what later opens the file and replaces `fitness`. The name and the string parse are fixed here so that change does not add a second parser.

### Tests

The handshake expects these lines, in this order, after the author line and before `uciok`:

```
option name Hash type spin default 64 min 1 max 1024
option name Threads type spin default 1 min 1 max 1
option name EvalFile type string default <empty>
```

`_tt` prints the transposition entry count. `_option <id>` prints the stored value of one of the three options: the clamped megabytes, `1` for threads, or the path. An empty path prints `<empty>`.

- No `setoption`, then `_tt`, prints 4194304. `setoption name Hash value 64` prints the same.
- `setoption name hash value 1`, `16`, and `3` print 65536, 1048576, and 131072.
- `setoption name Hash value 0` and `value 100000` print 65536 and 67108864.
- `setoption name Threads value 4`, then the mate-in-one search, still plays `d8h4`. `_option Threads` prints 1.
- `setoption name EvalFile value nets/my net.nnue` stores that path, including the space. `_eval` of the start position is the same number as before the option.
- `setoption name Ponder value true`, then `isready`, prints `readyok` and nothing else.
- Mate in one, the fastest mate, and the fork are run without a `setoption`, and their moves stay put.

Perft is unchanged.

### README

The command list includes `setoption`. One sentence says `Hash` is the transposition table in megabytes (default 64), `Threads` is 1, and `EvalFile` remembers a weight-file path while the hand-written eval still scores the leaf. The debug list includes `_tt` and `_option`.

### Out of scope

- Opening a weight file, and replacing `Evaluation::fitness`.
- A second search thread, and any `Threads` value other than 1.
- The pawn/king table, `Clear Hash`, `Ponder`, `MultiPV`, `OwnBook`, and the `UCI_` options.
- Applying an option from `pollSearchInput`.
- Changing the 64 MiB default, the 1 MiB minimum, or the 1024 MiB cap. Those are the first numbers.
- `scripts/elo.sh`. A match can pass `option.Hash` and `option.Threads` once the engine announces them.

## Consequences

- A GUI can set the table size, can send `Threads`, and can name a weight file before a network exists to load it.
- A session that never sends `setoption` keeps today's table and today's moves. `go depth N` at another `Hash` can return a different move, because a different number of slots changes which entry is replaced.
- `EvalFile` does not change the score. The network is still deferred, as [0001](0001-port-tactician-engine.md), [0005](0005-search-scores.md), and [0006](0006-time-control.md) left it. When it arrives, it reads this option and replaces `fitness`. It does not add a new command parser.
- `Threads` does not add a worker. [0006](0006-time-control.md) still describes the search thread.
