#!/usr/bin/env python3
# Tests taken from https://chessprogramming.org/Perft_Results
# --smoke runs a shallow initial-position check. The default is the full suite.

import sys

from driver import run

FULL = [
    {
        'name': 'Initial',
        'fen': 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1',
        'depth': 6,
        'answer': 119060324,
    },
    {
        'name': 'Kiwi Pete',
        'fen': 'r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1',
        'depth': 5,
        'answer': 193690690,
    },
    {
        'name': 'Position 3',
        'fen': '8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1',
        'depth': 7,
        'answer': 178633661,
    },
    {
        'name': 'Position 4',
        'fen': 'r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1',
        'depth': 5,
        'answer': 15833292,
    },
    {
        'name': 'Position 4 Mirrored',
        'fen': 'r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1',
        'depth': 5,
        'answer': 15833292,
    },
    {
        'name': 'Position 5',
        'fen': 'rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8',
        'depth': 5,
        'answer': 89941194,
    },
    {
        'name': 'Position 6',
        'fen': 'r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10',
        'depth': 5,
        'answer': 164075551,
    },
]

SMOKE = [
    {
        'name': 'Initial depth 4',
        'fen': 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1',
        'depth': 4,
        'answer': 197281,
    },
]


def main():
    tests = SMOKE if '--smoke' in sys.argv else FULL
    commands = ['_perft %s %s' % (test['depth'], test['fen']) for test in tests]
    timeout = 60 if '--smoke' in sys.argv else 1800
    lines = run(commands, timeout=timeout)
    if len(lines) != len(tests):
        raise Exception('Expected %s perft lines, got %s' % (len(tests), lines))
    for test, line in zip(tests, lines):
        answer, time_ms = line.split(' ')
        if answer != str(test['answer']):
            raise Exception('Test failed: %s, %s, %s' % (test['name'], answer, test['answer']))
        print(test['name'] + ': ' + time_ms + 'ms')


if __name__ == '__main__':
    main()
