#!/usr/local/bin/python3
# Engine tests ported from tactician's TestBrain and TestEvaluation.

from subprocess import Popen, PIPE

ENGINE = '../src/neptune'


def run(commands):
    process = Popen([ENGINE], stdout=PIPE, stdin=PIPE, stderr=PIPE)
    script = '\n'.join(commands) + '\nquit\n'
    out, err = process.communicate(input=script.encode('ascii'))
    if process.returncode != 0:
        raise Exception('Engine exited %s: %s' % (process.returncode, err.decode('ascii', 'replace')))
    if err:
        raise Exception(err.decode('ascii', 'replace'))
    return out.decode('ascii').strip().split('\n')


def bestmove(commands):
    lines = run(commands)
    for line in lines:
        if line.startswith('bestmove '):
            return line.split(' ')[1]
    raise Exception('No bestmove in %s' % lines)


def value(command, fen):
    lines = run([command + ' ' + fen])
    return float(lines[-1])


def expect_best(name, commands, move):
    found = bestmove(commands)
    if found != move:
        raise Exception('%s: expected %s, got %s' % (name, move, found))
    print(name + ': ' + found)


def expect_order(name, scores):
    for earlier, later in zip(scores, scores[1:]):
        if not earlier > later:
            raise Exception('%s: expected decreasing scores, got %s' % (name, scores))
    print(name + ': ok')


def main():
    start = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'

    expect_best('mate in one', [
        'position startpos moves f2f3 e7e5 g2g4',
        'go depth 4',
    ], 'd8h4')

    expect_best('fastest mate', [
        'position fen 4K3/7r/5p2/5k2/8/8/1r6/8 b - - 0 1',
        'go depth 4',
    ], 'b2b8')

    # The queen is already on h4. Tactician's test reached this position by moving
    # the queen without checking that the path was clear.
    expect_best('fork', [
        'position fen rnb1kbnr/pp2pppp/2p5/1N1p4/7q/7P/PPPPPPP1/R1BQKBNR w KQkq - 0 4',
        'go depth 4',
    ], 'b5c7')

    losing = bestmove([
        'position fen 8/8/8/8/8/7k/q7/7K w - - 0 1',
        'go depth 3',
    ])
    if len(losing) < 4:
        raise Exception('losing position returned no move: ' + losing)
    print('losing position: ' + losing)

    overflow = bestmove([
        'position fen Bn5r/p3k2p/b5p1/4p3/3q4/2nP3Q/PPP2PPP/R3KR2 w Q - 5 19',
        'go depth 3',
    ])
    if len(overflow) < 4:
        raise Exception('table position returned no move: ' + overflow)
    print('table bounds: ' + overflow)

    opening = [
        value('_king w 0', 'rnbqrbnk/pppppppp/8/8/8/8/PPPPPPPP/RNBQRBNK w KQkq - 0 1'),
        value('_king w 0', 'rnbqrbnk/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'),
        value('_king w 0', 'rnbqrbnk/pppppppp/8/8/7K/8/PPPPPPPP/RNBQ1BNR w KQkq - 0 1'),
        value('_king w 0', 'rnbqrbnk/pppppppp/8/8/4K3/8/PPPPPPPP/RNBQ1BNR w KQkq - 0 1'),
    ]
    expect_order('king safety opening', opening)

    endgame = [
        value('_king w 1', 'k7/p7/8/8/3K4/8/P7/8 w - - 0 1'),
        value('_king w 1', 'k7/p7/8/8/K7/8/P7/8 w - - 0 1'),
        value('_king w 1', 'k7/p7/8/8/8/8/P7/3K4 w - - 0 1'),
        value('_king w 1', 'k7/p7/8/8/8/8/P7/K7 w - - 0 1'),
    ]
    expect_order('king safety endgame', endgame)

    shield = [
        value('_king w 0.3', '6k1/pppppppp/8/8/8/8/5PPP/6K1 w - - 0 1'),
        value('_king w 0.3', '6k1/pppppppp/8/8/6P1/8/5P1P/6K1 w - - 0 1'),
        value('_king w 0.3', '6k1/pppppppp/8/8/8/8/5P1P/6K1 w - - 0 1'),
    ]
    expect_order('pawn shield', shield)

    doubled = value('_eval', '7k/8/8/8/8/P7/PP6/7K w - - 0 1')
    separate = value('_eval', '7k/8/8/8/8/P7/1PP5/7K w - - 0 1')
    if not doubled < separate:
        raise Exception('doubled pawns scored %s, separate pawns %s' % (doubled, separate))
    print('doubled pawns: ok')

    isolated = value('_eval', '7k/8/8/8/8/8/P1P5/7K w - - 0 1')
    connected = value('_eval', '7k/8/8/8/8/8/PP6/7K w - - 0 1')
    if not isolated < connected:
        raise Exception('isolated pawns scored %s, connected pawns %s' % (isolated, connected))
    print('isolated pawns: ok')

    passed = value('_eval', '7k/8/8/8/2p5/8/P1P5/7K w - - 0 1')
    blocked = value('_eval', '7k/8/8/8/1p6/8/P1P5/7K w - - 0 1')
    if not passed > blocked:
        raise Exception('passed pawns scored %s, blocked pawns %s' % (passed, blocked))
    print('passed pawns: ok')

    both = value('_eval', 'rnbqrbnk/pppppppp/8/8/8/8/PPPPPPPP/RNBQRBNK w KQkq - 0 1')
    kingside = value('_eval', 'rnbqrbnk/pppppppp/8/8/8/8/PPPPPPPP/RNBQRBNK w Kkq - 0 1')
    queenside = value('_eval', 'rnbqrbnk/pppppppp/8/8/8/8/PPPPPPPP/RNBQRBNK w Qkq - 0 1')
    neither = value('_eval', 'rnbqrbnk/pppppppp/8/8/8/8/PPPPPPPP/RNBQRBNK w kq - 0 1')
    if not (both > kingside and both > queenside and kingside > neither and queenside > neither):
        raise Exception('castle rights %s %s %s %s' % (both, kingside, queenside, neither))
    print('castle rights: ok')

    broken = value('_eval', 'rnbqrbnk/pppppppp/8/8/6P1/8/PPPPPP1P/RNBQRBNK w KQkq - 0 1')
    intact = value('_eval', 'rnbqrbnk/pppppppp/8/8/8/8/PPPPPPPP/RNBQRBNK w KQkq - 0 1')
    if not broken < intact:
        raise Exception('broken shield %s, intact %s' % (broken, intact))
    print('castle shield: ok')

    down = value('_eval', 'rnbqkbnr/ppp1pppp/8/3P4/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2')
    up = value('_eval', 'rnbqkbnr/ppp1ppp1/7p/3P4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3')
    if not (down < 0 and up > 0):
        raise Exception('material scores %s %s' % (down, up))
    print('material: ok')

    start_fraction = value('_eg', start)
    end_fraction = value('_eg', '4k3/8/8/8/8/8/8/4K3 b - - 0 1')
    if not (abs(start_fraction) < 0.01 and abs(end_fraction - 1) < 0.01):
        raise Exception('endgame fraction %s %s' % (start_fraction, end_fraction))
    print('endgame fraction: ok')

    def hashes(commands):
        lines = run(commands + ['_hash'])
        return lines[-1]

    if run(['position startpos', '_roundtrip'])[-1] != 'ok':
        raise Exception('startpos roundtrip failed')
    if run(['position fen rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1', '_roundtrip'])[-1] != 'ok':
        raise Exception('en passant roundtrip failed')
    print('hash roundtrip: ok')

    after_e4 = hashes(['position startpos moves e2e4'])
    fen_e4 = hashes(['position fen rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1'])
    if after_e4 != fen_e4:
        raise Exception('e4 hash mismatch %s %s' % (after_e4, fen_e4))

    line_a = hashes(['position startpos moves e2e4 e7e5 g1f3 b8c6 d2d3'])
    line_b = hashes(['position startpos moves e2e4 e7e5 d2d3 b8c6 g1f3'])
    fen_line = hashes(['position fen r1bqkbnr/pppp1ppp/2n5/4p3/4P3/3P1N2/PPP2PPP/RNBQKB1R b KQkq - 1 3'])
    if not (line_a == line_b == fen_line):
        raise Exception('path hash mismatch %s %s %s' % (line_a, line_b, fen_line))
    print('hash paths: ok')


if __name__ == '__main__':
    main()
