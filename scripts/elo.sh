#!/usr/bin/env bash
# Play Neptune against Stockfish limited to a chosen Elo.
# Usage: ./scripts/elo.sh [elo]
# `make elo` builds Neptune and runs this.

set -euo pipefail

elo="${1:-2200}"
root="$(cd "$(dirname "$0")/.." && pwd)"
engine="${NEPTUNE:-$root/build/neptune}"
book="$root/openings.pgn"

if [[ ! "$elo" =~ ^[0-9]+$ ]]; then
	printf 'Usage: %s [elo]\n' "$0" >&2
	exit 1
fi

if ! command -v cutechess-cli >/dev/null 2>&1; then
	cat >&2 <<'EOF'
cutechess-cli is not on PATH.
It plays two engines against each other and records the score.
Get a current build from https://github.com/cutechess/cutechess/releases
On macOS, build it: https://github.com/cutechess/cutechess/wiki/Building-from-source
EOF
	exit 1
fi

if ! command -v stockfish >/dev/null 2>&1; then
	cat >&2 <<'EOF'
stockfish is not on PATH.
https://stockfishchess.org/download/
On macOS: brew install stockfish
EOF
	exit 1
fi

if [[ ! -x "$engine" ]]; then
	printf 'No engine at %s. Run make, or set NEPTUNE.\n' "$engine" >&2
	exit 1
fi

if [[ ! -f "$book" ]]; then
	zip="$root/8moves_v3.pgn.zip"
	printf 'Downloading opening book...\n'
	curl -fL --retry 3 -o "$zip" \
		https://github.com/official-stockfish/books/raw/master/8moves_v3.pgn.zip
	unzip -q -o "$zip" -d "$root"
	rm -f "$zip"
	if [[ ! -f "$root/8moves_v3.pgn" ]]; then
		printf 'The archive did not contain 8moves_v3.pgn.\n' >&2
		exit 1
	fi
	mv "$root/8moves_v3.pgn" "$book"
fi

cd "$root"
cutechess-cli \
	-engine name=neptune cmd="$engine" \
	-engine name="sf_${elo}" cmd=stockfish option.Threads=1 option.UCI_LimitStrength=true option.UCI_Elo="$elo" \
	-each proto=uci tc=60+0.6 \
	-openings file=openings.pgn format=pgn order=random \
	-repeat -games 2 -rounds 8 -concurrency 4 \
	-resign movecount=3 score=600 \
	-draw movenumber=40 movecount=8 score=10 \
	-ratinginterval 4 \
	-recover -pgnout "games_${elo}.pgn" \
	2>&1 | tee "log_${elo}.txt"
