#!/usr/bin/env python3
# Engine tests ported from tactician's TestBrain and TestEvaluation.
# One process covers the suite. ucinewgame before each search keeps the
# transposition table from leaking between cases.

from driver import run, run_until_eof


class Out:
    def __init__(self, lines):
        self.lines = lines
        self.i = 0

    def next(self):
        if self.i >= len(self.lines):
            raise Exception('Unexpected end of output, after %s' % self.lines)
        line = self.lines[self.i]
        self.i += 1
        return line

    def done(self):
        if self.i != len(self.lines):
            raise Exception('Leftover output: %s' % self.lines[self.i:])


def expect_info(info, name):
    if not info.startswith('info ') or 'depth' not in info or 'score cp' not in info or ' pv' not in info:
        raise Exception('%s: info line missing fields: %s' % (name, info))


def expect_best(out, name, move):
    info = out.next()
    found = out.next()
    expect_info(info, name)
    if not found.startswith('bestmove '):
        raise Exception('%s: expected bestmove, got %s' % (name, found))
    got = found.split(' ')[1]
    if got != move:
        raise Exception('%s: expected %s, got %s' % (name, move, got))
    print(name + ': ' + got)


def expect_some_move(out, name):
    info = out.next()
    found = out.next()
    expect_info(info, name)
    if not found.startswith('bestmove '):
        raise Exception('%s: expected bestmove, got %s' % (name, found))
    move = found.split(' ')[1]
    if len(move) < 4:
        raise Exception('%s returned no move: %s' % (name, move))
    print(name + ': ' + move)


def expect_line(out, expected, name):
    found = out.next()
    if found != expected:
        raise Exception('%s: expected %s, got %s' % (name, expected, found))


def float_line(out):
    return float(out.next())


def expect_order(name, scores):
    for earlier, later in zip(scores, scores[1:]):
        if not earlier > later:
            raise Exception('%s: expected decreasing scores, got %s' % (name, scores))
    print(name + ': ok')


def main():
    start = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'
    commands = [
        'uci',
        '',
        'isready',
        'position fen 4k3/8/8/8/8/8/8/4K3 w - - 0 1',
        'ucinewgame',
        '_hash',
        'position startpos',
        '_hash',
        'ucinewgame',
        'position startpos moves f2f3 e7e5 g2g4',
        'go depth 6',
        'ucinewgame',
        'position fen 4K3/7r/5p2/5k2/8/8/1r6/8 b - - 0 1',
        'go depth 6',
        'ucinewgame',
        # The queen is already on h4. Tactician's test reached this position by
        # moving the queen without checking that the path was clear.
        'position fen rnb1kbnr/pp2pppp/2p5/1N1p4/7q/7P/PPPPPPP1/R1BQKBNR w KQkq - 0 4',
        'go depth 6',
        'ucinewgame',
        'position fen 8/8/8/8/8/7k/q7/7K w - - 0 1',
        'go depth 3',
        'ucinewgame',
        'position fen Bn5r/p3k2p/b5p1/4p3/3q4/2nP3Q/PPP2PPP/R3KR2 w Q - 5 19',
        'go depth 3',
    ]

    king_fens = [
        ('_king w 0', 'rnbqrbnk/pppppppp/8/8/8/8/PPPPPPPP/RNBQRBNK w KQkq - 0 1'),
        ('_king w 0', 'rnbqrbnk/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'),
        ('_king w 0', 'rnbqrbnk/pppppppp/8/8/7K/8/PPPPPPPP/RNBQ1BNR w KQkq - 0 1'),
        ('_king w 0', 'rnbqrbnk/pppppppp/8/8/4K3/8/PPPPPPPP/RNBQ1BNR w KQkq - 0 1'),
        ('_king w 1', 'k7/p7/8/8/3K4/8/P7/8 w - - 0 1'),
        ('_king w 1', 'k7/p7/8/8/K7/8/P7/8 w - - 0 1'),
        ('_king w 1', 'k7/p7/8/8/8/8/P7/3K4 w - - 0 1'),
        ('_king w 1', 'k7/p7/8/8/8/8/P7/K7 w - - 0 1'),
        ('_king w 0.3', '6k1/pppppppp/8/8/8/8/5PPP/6K1 w - - 0 1'),
        ('_king w 0.3', '6k1/pppppppp/8/8/6P1/8/5P1P/6K1 w - - 0 1'),
        ('_king w 0.3', '6k1/pppppppp/8/8/8/8/5P1P/6K1 w - - 0 1'),
    ]
    for command, fen in king_fens:
        commands.append(command + ' ' + fen)

    eval_fens = [
        '7k/8/8/8/8/P7/PP6/7K w - - 0 1',
        '7k/8/8/8/8/P7/1PP5/7K w - - 0 1',
        '7k/8/8/8/8/8/P1P5/7K w - - 0 1',
        '7k/8/8/8/8/8/PP6/7K w - - 0 1',
        '7k/8/8/8/2p5/8/P1P5/7K w - - 0 1',
        '7k/8/8/8/1p6/8/P1P5/7K w - - 0 1',
        'rnbqrbnk/pppppppp/8/8/8/8/PPPPPPPP/RNBQRBNK w KQkq - 0 1',
        'rnbqrbnk/pppppppp/8/8/8/8/PPPPPPPP/RNBQRBNK w Kkq - 0 1',
        'rnbqrbnk/pppppppp/8/8/8/8/PPPPPPPP/RNBQRBNK w Qkq - 0 1',
        'rnbqrbnk/pppppppp/8/8/8/8/PPPPPPPP/RNBQRBNK w kq - 0 1',
        'rnbqrbnk/pppppppp/8/8/6P1/8/PPPPPP1P/RNBQRBNK w KQkq - 0 1',
        'rnbqrbnk/pppppppp/8/8/8/8/PPPPPPPP/RNBQRBNK w KQkq - 0 1',
        'rnbqkbnr/ppp1pppp/8/3P4/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2',
        'rnbqkbnr/ppp1ppp1/7p/3P4/8/8/PPPP1PPP/RNBQKBNR w KQkq - 0 3',
        '4k3/8/8/8/8/8/8/2B1KB2 w - - 0 1',
        '4k3/8/8/8/8/8/8/2B1KN2 w - - 0 1',
        '4k3/1p6/8/8/8/8/8/R3K3 w - - 0 1',
        '4k3/1p6/8/8/8/8/8/1R2K3 w - - 0 1',
        '4k3/8/8/8/8/8/1P6/R3K3 w - - 0 1',
        '4k3/8/8/8/8/8/1P6/1R2K3 w - - 0 1',
    ]
    for fen in eval_fens:
        commands.append('_eval ' + fen)

    commands.append('_eg ' + start)
    commands.append('_eg 4k3/8/8/8/8/8/8/4K3 b - - 0 1')
    commands += [
        'position startpos',
        '_roundtrip',
        'position fen rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1',
        '_roundtrip',
        'position startpos moves e2e4',
        '_hash',
        'position fen rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1',
        '_hash',
        'position startpos moves e2e4 e7e5 g1f3 b8c6 d2d3',
        '_hash',
        'position startpos moves e2e4 e7e5 d2d3 b8c6 g1f3',
        '_hash',
        'position fen r1bqkbnr/pppp1ppp/2n5/4p3/4P3/3P1N2/PPP2PPP/RNBQKB1R b KQkq - 1 3',
        '_hash',
        'position fen r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1 moves e1g1',
        '_hash',
        'position fen r3k2r/8/8/8/8/8/8/R4RK1 b kq - 0 1',
        '_hash',
        'position fen 4k3/P7/8/8/8/8/8/4K3 w - - 0 1 moves a7a8q',
        '_hash',
        'position fen Q3k3/8/8/8/8/8/8/4K3 b - - 0 1',
        '_hash',
    ]

    out = Out(run(commands))

    expect_line(out, 'id name Neptune', 'uci')
    expect_line(out, 'id author Phil Leszczynski', 'uci')
    expect_line(out, 'uciok', 'uci')
    expect_line(out, 'readyok', 'isready')
    print('uci handshake: ok')

    fresh = out.next()
    start_hash = out.next()
    if fresh != start_hash:
        raise Exception('ucinewgame hash %s, startpos %s' % (fresh, start_hash))
    print('ucinewgame: ok')

    expect_best(out, 'mate in one', 'd8h4')
    expect_best(out, 'fastest mate', 'b2b8')
    expect_best(out, 'fork', 'b5c7')
    expect_some_move(out, 'losing position')
    expect_some_move(out, 'table bounds')

    opening = [float_line(out) for _ in range(4)]
    expect_order('king safety opening', opening)
    endgame = [float_line(out) for _ in range(4)]
    expect_order('king safety endgame', endgame)
    shield = [float_line(out) for _ in range(3)]
    expect_order('pawn shield', shield)

    doubled, separate = float_line(out), float_line(out)
    if not doubled < separate:
        raise Exception('doubled pawns scored %s, separate pawns %s' % (doubled, separate))
    print('doubled pawns: ok')

    isolated, connected = float_line(out), float_line(out)
    if not isolated < connected:
        raise Exception('isolated pawns scored %s, connected pawns %s' % (isolated, connected))
    print('isolated pawns: ok')

    passed, blocked = float_line(out), float_line(out)
    if not passed > blocked:
        raise Exception('passed pawns scored %s, blocked pawns %s' % (passed, blocked))
    print('passed pawns: ok')

    both, kingside, queenside, neither = (float_line(out) for _ in range(4))
    if not (both > kingside and both > queenside and kingside > neither and queenside > neither):
        raise Exception('castle rights %s %s %s %s' % (both, kingside, queenside, neither))
    print('castle rights: ok')

    broken, intact = float_line(out), float_line(out)
    if not broken < intact:
        raise Exception('broken shield %s, intact %s' % (broken, intact))
    print('castle shield: ok')

    down, up = float_line(out), float_line(out)
    if not (down < 0 and up > 0):
        raise Exception('material scores %s %s' % (down, up))
    print('material: ok')

    pair, single = float_line(out), float_line(out)
    if not pair > single:
        raise Exception('bishop pair %s, bishop and knight %s' % (pair, single))
    print('bishop pair: ok')

    rook_open, rook_semi = float_line(out), float_line(out)
    rook_open_pawn, rook_closed = float_line(out), float_line(out)
    if not (rook_open > rook_semi and rook_open_pawn > rook_closed):
        raise Exception('rook files %s %s %s %s' % (rook_open, rook_semi, rook_open_pawn, rook_closed))
    print('rook files: ok')

    start_fraction, end_fraction = float_line(out), float_line(out)
    if not (abs(start_fraction) < 0.01 and abs(end_fraction - 1) < 0.01):
        raise Exception('endgame fraction %s %s' % (start_fraction, end_fraction))
    print('endgame fraction: ok')

    if out.next() != 'ok' or out.next() != 'ok':
        raise Exception('hash roundtrip failed')
    print('hash roundtrip: ok')

    after_e4, fen_e4 = out.next(), out.next()
    if after_e4 != fen_e4:
        raise Exception('e4 hash mismatch %s %s' % (after_e4, fen_e4))
    line_a, line_b, fen_line = out.next(), out.next(), out.next()
    if not (line_a == line_b == fen_line):
        raise Exception('path hash mismatch %s %s %s' % (line_a, line_b, fen_line))
    print('hash paths: ok')

    castled, castled_fen = out.next(), out.next()
    if castled != castled_fen:
        raise Exception('castle hash mismatch %s %s' % (castled, castled_fen))
    print('castling move: ok')

    promoted, promoted_fen = out.next(), out.next()
    if promoted != promoted_fen:
        raise Exception('promotion hash mismatch %s %s' % (promoted, promoted_fen))
    print('promotion move: ok')

    out.done()

    eof_lines = run_until_eof(['isready'])
    if eof_lines != ['readyok']:
        raise Exception('eof output: %s' % eof_lines)
    print('eof: ok')


if __name__ == '__main__':
    main()
