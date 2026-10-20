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
    has_score = 'score cp' in info or 'score mate' in info
    if not info.startswith('info ') or 'depth' not in info or not has_score or ' pv' not in info:
        raise Exception('%s: info line missing fields: %s' % (name, info))
    if ' nodes ' not in info or ' time ' not in info:
        raise Exception('%s: info line missing nodes or time: %s' % (name, info))


def last_search(out, name):
    info = None
    while True:
        line = out.next()
        if line.startswith('info '):
            info = line
            continue
        if line.startswith('bestmove '):
            if info is None:
                raise Exception('%s: no info line before bestmove' % name)
            return info, line
        raise Exception('%s: expected info or bestmove, got %s' % (name, line))


def info_depth(info, name):
    expect_info(info, name)
    parts = info.split()
    try:
        return int(parts[parts.index('depth') + 1])
    except (ValueError, IndexError):
        raise Exception('%s: no depth in %s' % (name, info))


def expect_best(out, name, move, depth=None):
    info, found = last_search(out, name)
    expect_info(info, name)
    if depth is not None and info_depth(info, name) != depth:
        raise Exception('%s: depth %s, expected %s' % (name, info_depth(info, name), depth))
    got = found.split(' ')[1]
    if got != move:
        raise Exception('%s: expected %s, got %s' % (name, move, got))
    print(name + ': ' + got)


def score_of(info, name):
    expect_info(info, name)
    parts = info.split()
    try:
        return int(parts[parts.index('cp') + 1])
    except (ValueError, IndexError):
        raise Exception('%s: no score in %s' % (name, info))


def expect_scored(out, name, move, score=None, score_min=None, score_max=None):
    info, found = last_search(out, name)
    value = score_of(info, name)
    if not found.startswith('bestmove '):
        raise Exception('%s: expected bestmove, got %s' % (name, found))
    got = found.split()[1]
    if move is not None and got != move:
        raise Exception('%s: expected %s, got %s (score %s)' % (name, move, got, value))
    if move is None and (len(got) < 4 or got == '(none)'):
        raise Exception('%s returned no move: %s' % (name, got))
    if score is not None and value != score:
        raise Exception('%s: score %s, expected %s (%s)' % (name, value, score, got))
    if score_min is not None and value < score_min:
        raise Exception('%s: score %s, expected at least %s (%s)' % (name, value, score_min, got))
    if score_max is not None and value > score_max:
        raise Exception('%s: score %s, expected at most %s (%s)' % (name, value, score_max, got))
    print('%s: %s %s' % (name, got, value))


def mate_of(info, name):
    expect_info(info, name)
    parts = info.split()
    try:
        return int(parts[parts.index('mate') + 1])
    except (ValueError, IndexError):
        raise Exception('%s: no mate score in %s' % (name, info))


def expect_mate(out, name, move, moves, depth=None):
    info, found = last_search(out, name)
    got = mate_of(info, name)
    if depth is not None and info_depth(info, name) != depth:
        raise Exception('%s: depth %s, expected %s' % (name, info_depth(info, name), depth))
    if not found.startswith('bestmove '):
        raise Exception('%s: expected bestmove, got %s' % (name, found))
    played = found.split(' ')[1]
    if played != move:
        raise Exception('%s: expected %s, got %s (mate %s)' % (name, move, played, got))
    if got != moves:
        raise Exception('%s: mate %s, expected %s' % (name, got, moves))
    print('%s: %s mate %s' % (name, played, got))


def expect_hashes(out, name, count):
    hashes = [out.next() for _ in range(count)]
    if any(item != hashes[0] for item in hashes):
        raise Exception('%s hash mismatch %s' % (name, hashes))
    print(name + ': ok')


def expect_some_move(out, name):
    info, found = last_search(out, name)
    expect_info(info, name)
    if not found.startswith('bestmove '):
        raise Exception('%s: expected bestmove, got %s' % (name, found))
    move = found.split(' ')[1]
    if len(move) < 4 or move == '(none)':
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
        # Same mate, far apart in the game. The score must not depend on fullmove.
        'position fen rnbqkbnr/pppp1ppp/8/4p3/6P1/5P2/PPPPP2P/RNBQKBNR b KQkq g3 0 1',
        'go depth 6',
        'ucinewgame',
        'position fen rnbqkbnr/pppp1ppp/8/4p3/6P1/5P2/PPPPP2P/RNBQKBNR b KQkq g3 0 40',
        'go depth 6',
        'ucinewgame',
        'position fen 4K3/7r/5p2/5k2/8/8/1r6/8 b - - 0 1',
        'go depth 6',
        'ucinewgame',
        'position fen 4K3/7r/5p2/5k2/8/8/1r6/8 b - - 0 40',
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
        'position fen 4k3/P7/8/8/8/8/8/4K3 w - - 0 1 moves a7a8Q',
        '_hash',
        'position fen 4k3/P7/8/8/8/8/8/4K3 w - - 0 1 moves a7a8q',
        '_hash',
        'position fen Q3k3/8/8/8/8/8/8/4K3 b - - 0 1',
        '_hash',
        'position fen k7/4P3/8/8/8/8/8/4K3 w - - 0 1 moves e7e8Q',
        '_hash',
        'position fen k7/4P3/8/8/8/8/8/4K3 w - - 0 1 moves e7e8q',
        '_hash',
        'position fen k3Q3/8/8/8/8/8/8/4K3 b - - 0 1',
        '_hash',
        'position fen k7/4P3/8/8/8/8/8/4K3 w - - 0 1 moves e7e8R',
        '_hash',
        'position fen k7/4P3/8/8/8/8/8/4K3 w - - 0 1 moves e7e8r',
        '_hash',
        'position fen k3R3/8/8/8/8/8/8/4K3 b - - 0 1',
        '_hash',
        'position fen k7/4P3/8/8/8/8/8/4K3 w - - 0 1 moves e7e8B',
        '_hash',
        'position fen k7/4P3/8/8/8/8/8/4K3 w - - 0 1 moves e7e8b',
        '_hash',
        'position fen k3B3/8/8/8/8/8/8/4K3 b - - 0 1',
        '_hash',
        'position fen k7/4P3/8/8/8/8/8/4K3 w - - 0 1 moves e7e8N',
        '_hash',
        'position fen k7/4P3/8/8/8/8/8/4K3 w - - 0 1 moves e7e8n',
        '_hash',
        'position fen k3N3/8/8/8/8/8/8/4K3 b - - 0 1',
        '_hash',
        'position fen k2r4/4P3/8/8/8/8/8/4K3 w - - 0 1 moves e7d8Q',
        '_hash',
        'position fen k2r4/4P3/8/8/8/8/8/4K3 w - - 0 1 moves e7d8q',
        '_hash',
        'position fen k2Q4/8/8/8/8/8/8/4K3 b - - 0 1',
        '_hash',
        'position fen 4k3/8/8/8/8/8/4p3/7K b - - 0 1 moves e2e1Q',
        '_hash',
        'position fen 4k3/8/8/8/8/8/4p3/7K b - - 0 1 moves e2e1q',
        '_hash',
        'position fen 4k3/8/8/8/8/8/8/4q2K w - - 0 2',
        '_hash',
        # Queen versus king. At clock 0 this is a win. At 99 every move is quiet,
        # the reply can claim the fifty-move draw, and the score is 0.
        'ucinewgame',
        'position fen 8/8/8/7k/8/8/Q7/K7 w - - 0 60',
        'go depth 3',
        'ucinewgame',
        'position fen 8/8/8/7k/8/8/Q7/K7 w - - 99 60',
        'go depth 3',
        # Mate in one still wins when the clock is already full.
        'ucinewgame',
        'position fen 7k/8/6K1/8/8/8/8/R7 w - - 100 1',
        'go depth 4',
        'ucinewgame',
        'position fen 7k/8/6K1/8/8/8/8/R7 w - - 100 80',
        'go depth 4',
        # Side to move is already checkmated.
        'ucinewgame',
        'position fen R6k/8/6K1/8/8/8/8/8 b - - 0 1',
        'go depth 1',
        'ucinewgame',
        'position fen 4k3/8/8/8/8/8/8/4K3 w - - 0 1',
        'go depth 2',
        'ucinewgame',
        'position fen 4k3/8/8/8/8/8/8/4KB2 w - - 0 1',
        'go depth 2',
        # Two visits are not a draw. The third visit of this queen-up position is.
        'ucinewgame',
        'position fen 4k3/8/8/8/8/8/8/3QK3 b - - 0 1 moves e8e7 d1d2 e7e8 d2d1 e8e7 d1d2 e7e8',
        'go depth 3',
        'ucinewgame',
        'position fen 4k3/8/8/8/8/8/8/3QK3 b - - 0 1 moves e8e7 d1d2 e7e8 d2d1 e8e7 d1d2 e7e8 d2d1',
        'go depth 2',
        # Quiet mate, a seventh-rank pawn, and a stalemate are horizon scores.
        'ucinewgame',
        'position startpos moves f2f3 e7e5 g2g4',
        'go depth 1',
        'ucinewgame',
        'position fen 7k/8/6K1/8/8/8/8/R7 w - - 0 1',
        'go depth 1',
        'ucinewgame',
        'position fen 4k3/P7/8/8/8/8/8/4K3 b - - 0 1',
        'go depth 1',
        'ucinewgame',
        'position fen k7/P7/2K5/8/8/8/8/8 w - - 0 1',
        'go depth 1',
        'ucinewgame',
        'position startpos moves f2f3 e7e5 g2g4 d8h4',
        'go depth 1',
        'ucinewgame',
        'position fen k7/P7/1K6/8/8/8/8/8 b - - 0 1',
        'go depth 1',
    ]

    out = Out(run(commands))

    expect_line(out, 'id name Neptune', 'uci')
    expect_line(out, 'id author Phil Leszczynski', 'uci')
    expect_line(out, 'option name Hash type spin default 64 min 1 max 1024', 'uci')
    expect_line(out, 'option name Threads type spin default 1 min 1 max 1', 'uci')
    expect_line(out, 'option name EvalFile type string default <empty>', 'uci')
    expect_line(out, 'uciok', 'uci')
    expect_line(out, 'readyok', 'isready')
    print('uci handshake: ok')

    fresh = out.next()
    start_hash = out.next()
    if fresh != start_hash:
        raise Exception('ucinewgame hash %s, startpos %s' % (fresh, start_hash))
    print('ucinewgame: ok')

    expect_mate(out, 'mate in one', 'd8h4', 1, depth=6)
    expect_mate(out, 'mate in one at fullmove 1', 'd8h4', 1)
    expect_mate(out, 'mate in one at fullmove 40', 'd8h4', 1)
    expect_mate(out, 'fastest mate', 'b2b8', 1)
    expect_mate(out, 'fastest mate at fullmove 40', 'b2b8', 1)
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

    expect_hashes(out, 'promotion move', 3)
    expect_hashes(out, 'promotion e7e8Q', 3)
    expect_hashes(out, 'promotion e7e8R', 3)
    expect_hashes(out, 'promotion e7e8B', 3)
    expect_hashes(out, 'promotion e7e8N', 3)
    expect_hashes(out, 'promotion e7d8Q', 3)
    expect_hashes(out, 'promotion e2e1Q', 3)

    expect_scored(out, 'queen versus king', None, score_min=400)
    expect_scored(out, 'fifty-move draw', None, score=0)
    expect_mate(out, 'mate before fifty-move claim at fullmove 1', 'a1a8', 1)
    expect_mate(out, 'mate before fifty-move claim', 'a1a8', 1)
    expect_mate(out, 'already checkmated', '(none)', 0)
    expect_scored(out, 'king versus king', None, score=0)
    expect_scored(out, 'king and bishop versus king', None, score=0)
    expect_scored(out, 'twofold is not a draw', None, score_min=400)
    expect_scored(out, 'threefold repetition', None, score=0)
    expect_mate(out, 'horizon mate in one', 'd8h4', 1)
    expect_mate(out, 'horizon rook mate', 'a1a8', 1)
    expect_scored(out, 'horizon promotion', None, score_max=-500)
    expect_scored(out, 'horizon stalemate', None, score=0)
    expect_mate(out, 'root checkmate', '(none)', 0)
    expect_scored(out, 'root stalemate', '(none)', score=0)

    out.done()

    eof_lines = run_until_eof(['isready'])
    if eof_lines != ['readyok']:
        raise Exception('eof output: %s' % eof_lines)
    print('eof: ok')

    options()
    time_control()


START_MOVES = {
    'a2a3', 'a2a4', 'b2b3', 'b2b4', 'c2c3', 'c2c4', 'd2d3', 'd2d4',
    'e2e3', 'e2e4', 'f2f3', 'f2f4', 'g2g3', 'g2g4', 'h2h3', 'h2h4',
    'b1a3', 'b1c3', 'g1f3', 'g1h3',
}


def search_lines(lines, name):
    infos = [line for line in lines if line.startswith('info ')]
    bests = [line for line in lines if line.startswith('bestmove ')]
    if not infos or len(bests) != 1:
        raise Exception('%s: expected info and one bestmove, got %s' % (name, lines))
    return infos[-1], bests[0]


def options():
    start = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1'
    lines = run([
        '_tt',
        'setoption name Hash value 64',
        '_tt',
        'setoption name hash value 1',
        '_tt',
        'setoption name hash value 16',
        '_tt',
        'setoption name Hash value lots',
        '_option Hash',
        '_tt',
        'setoption name hash value 3',
        '_option Hash',
        '_tt',
        'setoption name Hash value 0',
        '_option Hash',
        '_tt',
        'setoption name Hash value 100000',
        '_option Hash',
        '_tt',
        'setoption name Threads value 4',
        'ucinewgame',
        'position startpos moves f2f3 e7e5 g2g4',
        'go depth 6',
        '_option Threads',
        '_eval ' + start,
        'setoption name EvalFile value nets/my net.nnue',
        '_option EvalFile',
        '_eval ' + start,
        'setoption name EvalFile value <empty>',
        '_option EvalFile',
        'setoption name Ponder value true',
        'isready',
    ])
    expected = [
        '4194304',
        '4194304',
        '65536',
        '1048576',
        '16',
        '1048576',
        '3',
        '131072',
        '1',
        '65536',
        '1024',
        '67108864',
    ]
    if lines[:len(expected)] != expected:
        raise Exception('options: expected %s, got %s' % (expected, lines[:len(expected)]))
    i = len(expected)
    info = None
    while i < len(lines) and lines[i].startswith('info '):
        info = lines[i]
        i += 1
    if info is None or i >= len(lines) or not lines[i].startswith('bestmove '):
        raise Exception('options: expected a search, got %s' % lines[i:])
    got = lines[i].split()[1]
    if got != 'd8h4':
        raise Exception('threads: expected d8h4, got %s' % got)
    i += 1
    if i >= len(lines) or lines[i] != '1':
        raise Exception('threads: expected 1, got %s' % lines[i:])
    i += 1
    if i + 2 >= len(lines):
        raise Exception('eval file: unexpected end %s' % lines[i:])
    before = lines[i]
    i += 1
    if lines[i] != 'nets/my net.nnue':
        raise Exception('eval file: expected path, got %s' % lines[i])
    i += 1
    if lines[i] != before:
        raise Exception('eval file changed the score %s %s' % (before, lines[i]))
    i += 1
    if lines[i:] != ['<empty>', 'readyok']:
        raise Exception('options tail: %s' % lines[i:])
    print('options: ok')


def time_control():
    mate_lines = run([
        'ucinewgame',
        'position startpos moves f2f3 e7e5 g2g4',
        'go movetime 0',
    ], timeout=30)
    info, best = search_lines(mate_lines, 'movetime 0')
    if info_depth(info, 'movetime 0') != 1:
        raise Exception('movetime 0: depth %s, expected 1' % info_depth(info, 'movetime 0'))
    if mate_of(info, 'movetime 0') != 1:
        raise Exception('movetime 0: mate %s, expected 1' % mate_of(info, 'movetime 0'))
    got = best.split()[1]
    if got != 'd8h4':
        raise Exception('movetime 0: expected d8h4, got %s' % got)
    print('movetime 0: ' + got)

    clock_lines = run([
        'position startpos',
        'go wtime 0 btime 0',
    ], timeout=30)
    info, best = search_lines(clock_lines, 'zero clock')
    if info_depth(info, 'zero clock') != 1:
        raise Exception('zero clock: depth %s, expected 1' % info_depth(info, 'zero clock'))
    got = best.split()[1]
    if got not in START_MOVES:
        raise Exception('zero clock: expected a legal move, got %s' % got)
    print('zero clock: ' + got)

    # stop is already in the pipe when the search starts. A search that never
    # reads it runs until the timeout.
    infinite_lines = run([
        'position startpos',
        'go infinite',
        'isready',
        'stop',
    ], timeout=10)
    try:
        ready_at = infinite_lines.index('readyok')
    except ValueError:
        raise Exception('infinite: no readyok in %s' % infinite_lines)
    info_at = next(i for i, line in enumerate(infinite_lines) if line.startswith('info '))
    best_at = next(i for i, line in enumerate(infinite_lines) if line.startswith('bestmove '))
    if not (ready_at < info_at < best_at):
        raise Exception('infinite: expected readyok, info, bestmove, got %s' % infinite_lines)
    got = infinite_lines[best_at].split()[1]
    if got not in START_MOVES:
        raise Exception('infinite: expected a legal move, got %s' % got)
    print('infinite stop: ' + got)


if __name__ == '__main__':
    main()
