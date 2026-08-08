# Messier self-play and SPRT harness

`selfplay.py` runs UCI engines against each other, records every game, and
reports results from engine A's perspective. It is meant for comparing a
candidate Messier build with a previous commit or with another UCI engine.

The harness currently provides:

- color-reversed opening pairs;
- fixed-node, fixed-depth, fixed-movetime, and clock time controls;
- legal-move and game-end validation through `python-chess`;
- time, crash, malformed-move, and illegal-move forfeits;
- W-D-L, score percentage, logistic Elo, a 95% confidence interval, and LOS;
- an optional trinomial generalized SPRT with configurable hypotheses;
- PGN, JSON Lines, configuration, and summary output for every run;
- parallel games, while keeping each individual game single-threaded unless
  the engine itself is configured otherwise.

This is intentionally a readable in-repository harness. For large-scale Elo
testing, compare its results with a mature runner such as `cutechess-cli`,
OpenBench, or fastchess before relying on it as the sole testing system.

## Install

Create a virtual environment and install the one runtime dependency:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r bench/requirements.txt
```

Build the current checkout:

```sh
meson setup build --buildtype=release
meson compile -C build
```

## Compare the current build with an earlier commit

A separate Git worktree keeps the two binaries independent:

```sh
git worktree add /tmp/messier-baseline HEAD~1
meson setup /tmp/messier-baseline/build /tmp/messier-baseline --buildtype=release
meson compile -C /tmp/messier-baseline/build
```

Run a short fixed-node smoke match:

```sh
.venv/bin/python bench/selfplay.py \
  --engine-a ./build/src/messier \
  --engine-b /tmp/messier-baseline/build/src/messier \
  --name-a candidate \
  --name-b baseline \
  --nodes 50000 \
  --games 20
```

Engine A is always the candidate. `W-D-L`, Elo, LOS, and SPRT signs are all
shown from engine A's perspective. Each opening is normally played twice,
with engine colors reversed in the second game.

## Run an SPRT

This example tests the hypotheses that the candidate is no better than the
baseline (`H0 = 0 Elo`) versus an improvement of at least 5 Elo (`H1 = 5 Elo`):

```sh
.venv/bin/python bench/selfplay.py \
  --engine-a ./build/src/messier \
  --engine-b /tmp/messier-baseline/build/src/messier \
  --nodes 100000 \
  --games 10000 \
  --sprt \
  --elo0 0 \
  --elo1 5 \
  --alpha 0.05 \
  --beta 0.05
```

The run stops after a completed batch when the log-likelihood ratio crosses
one of these boundaries:

- `accept H1`: evidence favors the improvement hypothesis;
- `accept H0`: evidence favors the null hypothesis;
- `continue`: neither bound has been reached.

The implementation uses W/D/L outcomes and maximizes the likelihood at each
Elo hypothesis over the unknown draw probability. That makes it a practical
trinomial generalized SPRT. It does not yet implement the pentanomial paired
model used by some high-volume chess testing systems. Color-reversed games are
scheduled as pairs, but the current likelihood calculation treats individual
game outcomes as observations.

SPRT bounds only make sense when chosen before the run. Do not repeatedly
change `elo0`, `elo1`, `alpha`, or `beta` after inspecting partial results.

## Search limits

Choose exactly one search limit. If none is supplied, the default is 50,000
nodes per move.

```sh
--nodes 100000       # deterministic work budget; good for search-quality tests
--depth 10           # equal nominal depth; generally less useful for Elo work
--movetime 100       # 100 milliseconds per move
--tc 10+0.1          # 10 seconds plus 0.1 seconds increment per move
```

Fixed-node matches mostly measure tree quality. Clock and movetime matches also
measure NPS, time management, and operating-system noise. For timing-sensitive
tests, start with `--concurrency 1`; high concurrency can distort results when
games compete for CPU time, cache, memory bandwidth, or thermal headroom.

The harness applies a small communication allowance before declaring a clock
loss. Change it with `--clock-grace-ms`. `--search-timeout` remains a hard
per-move watchdog for hung engines and may need to be raised for long controls.

## Openings

Without `--openings`, every pair starts from the normal initial position.
Supplying a varied, balanced opening set reduces bias and produces much more
useful results.

The harness accepts:

- a PGN file, taking the first `--opening-plies` plies of every game;
- a text file containing one FEN or EPD position per non-comment line.

Example:

```sh
.venv/bin/python bench/selfplay.py \
  --engine-a ./build/src/messier \
  --engine-b ./stockfish \
  --openings bench/openings.epd \
  --nodes 100000 \
  --games 200 \
  --seed 42
```

The opening order is deterministic for a given seed. Openings are shuffled
without replacement and recycled if the requested match is longer than the
book. Use an even game count so every position receives its reverse-color game.

## Engine commands and options

Quote commands that contain arguments:

```sh
--engine-a "./candidate --some-flag" --engine-b ./baseline
```

Set UCI options independently for each engine by repeating `--option-a` or
`--option-b`:

```sh
--option-a Hash=64 --option-b Hash=64
```

Use `--cwd-a` and `--cwd-b` when an engine needs to start in a particular
working directory. Use `--verbose` to print all UCI traffic when diagnosing a
protocol or timeout failure.

## Results

Each run creates `bench/results/<UTC timestamp>/` unless `--output-dir` is
given. It contains:

- `config.json`: commands, options, limits, hypotheses, and opening metadata;
- `games.jsonl`: one machine-readable record per game;
- `games.pgn`: replayable games with termination reasons;
- `summary.json`: final W-D-L, Elo, confidence interval, LOS, and SPRT state.

The reported Elo is the logistic conversion of the observed score rate:

```text
Elo = 400 * log10(score / (1 - score))
```

The confidence interval uses the observed variance of win/draw/loss game
scores. It is an asymptotic estimate, so very short matches and all-win or
all-draw samples should not be interpreted literally. Engine strength testing
is noisy: small changes usually require hundreds or thousands of games.

## Tests

The statistics and parser smoke tests use only the standard library:

```sh
python3 -m unittest bench.test_selfplay
python3 -m py_compile bench/selfplay.py
```

## Current scope and useful next steps

The initial harness favors clarity over every tournament-manager feature.
Likely extensions are:

1. pentanomial SPRT based on complete opening pairs;
2. evaluation-based draw and resignation adjudication;
3. recovery/resume from an existing JSONL result file;
4. Syzygy adjudication;
5. CPU affinity and per-engine resource controls;
6. Chess960 and other variants;
7. automatic build/worktree management for Git revisions.
