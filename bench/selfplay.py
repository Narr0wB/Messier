#!/usr/bin/env python3
"""Run paired UCI self-play matches and report Elo and SPRT statistics.

Engine A is the candidate and every score is reported from its perspective.
The harness deliberately depends only on python-chess outside the standard
library.  It is intended to be a transparent starting point that can grow with
Messier instead of replacing mature tournament managers such as cutechess-cli.
"""

from __future__ import annotations

import argparse
import collections
import concurrent.futures
import dataclasses
import datetime as dt
import json
import math
from pathlib import Path
import queue
import random
import re
import shlex
import subprocess
import sys
import threading
import time
from typing import Callable, Sequence

try:
    import chess
    import chess.pgn
except ModuleNotFoundError:
    chess = None


MATE_SCORE_CP = 100_000
TC_PATTERN = re.compile(r"^(?P<base>\d+(?:\.\d+)?)\+(?P<increment>\d+(?:\.\d+)?)$")


class HarnessError(RuntimeError):
    """A user-facing configuration or engine protocol error."""


class EngineError(HarnessError):
    """An engine process failed or violated the UCI protocol."""


@dataclasses.dataclass(frozen=True)
class EngineConfig:
    name: str
    command: tuple[str, ...]
    cwd: str | None
    options: tuple[tuple[str, str], ...]


@dataclasses.dataclass(frozen=True)
class SearchLimit:
    mode: str
    value: int | None = None
    base_ms: int | None = None
    increment_ms: int = 0

    def go_command(self, clocks: dict[bool, int] | None) -> str:
        if self.mode == "nodes":
            return f"go nodes {self.value}"
        if self.mode == "depth":
            return f"go depth {self.value}"
        if self.mode == "movetime":
            return f"go movetime {self.value}"
        if self.mode == "clock":
            assert clocks is not None
            return (
                f"go wtime {max(0, clocks[True])} btime {max(0, clocks[False])} "
                f"winc {self.increment_ms} binc {self.increment_ms}"
            )
        raise AssertionError(f"unknown search-limit mode: {self.mode}")

    def move_timeout(
        self,
        side: bool,
        clocks: dict[bool, int] | None,
        hard_timeout: float,
        clock_grace_ms: int,
    ) -> float:
        if self.mode == "clock":
            assert clocks is not None
            natural_timeout = max(2.0, clocks[side] / 1000.0 + clock_grace_ms / 1000.0 + 2.0)
            return min(hard_timeout, natural_timeout)
        if self.mode == "movetime":
            natural_timeout = max(2.0, (self.value or 0) / 1000.0 * 5.0 + 2.0)
            return min(hard_timeout, natural_timeout)
        return hard_timeout

    @property
    def label(self) -> str:
        if self.mode == "clock":
            return f"{self.base_ms / 1000:g}+{self.increment_ms / 1000:g}"
        return f"{self.mode}={self.value}"


@dataclasses.dataclass(frozen=True)
class SearchResult:
    move: str | None
    elapsed_ms: int
    score_cp: int | None


@dataclasses.dataclass(frozen=True)
class GameSpec:
    number: int
    opening_number: int
    fen: str
    candidate_is_white: bool


@dataclasses.dataclass
class GameRecord:
    number: int
    opening_number: int
    initial_fen: str
    white: str
    black: str
    candidate_is_white: bool
    result: str
    candidate_score: float
    termination: str
    plies: int
    moves: list[str]
    elapsed_ms: int
    final_clocks_ms: dict[str, int] | None
    pgn: str

    def json_dict(self) -> dict[str, object]:
        data = dataclasses.asdict(self)
        data.pop("pgn")
        return data


@dataclasses.dataclass
class MatchScore:
    wins: int = 0
    draws: int = 0
    losses: int = 0

    @property
    def games(self) -> int:
        return self.wins + self.draws + self.losses

    @property
    def points(self) -> float:
        return self.wins + 0.5 * self.draws

    @property
    def score_rate(self) -> float:
        return self.points / self.games if self.games else 0.5

    def add(self, score: float) -> None:
        if score == 1.0:
            self.wins += 1
        elif score == 0.5:
            self.draws += 1
        elif score == 0.0:
            self.losses += 1
        else:
            raise ValueError(f"invalid game score: {score}")

    def score_standard_error(self) -> float:
        if self.games < 2:
            return math.inf
        mean = self.score_rate
        sum_squares = self.wins + 0.25 * self.draws
        sample_variance = max(0.0, (sum_squares - self.games * mean * mean) / (self.games - 1))
        return math.sqrt(sample_variance / self.games)


@dataclasses.dataclass(frozen=True)
class SprtConfig:
    elo0: float
    elo1: float
    alpha: float
    beta: float

    @property
    def lower_bound(self) -> float:
        return math.log(self.beta / (1.0 - self.alpha))

    @property
    def upper_bound(self) -> float:
        return math.log((1.0 - self.beta) / self.alpha)


class UciEngine:
    """Small synchronous UCI client with timeout and crash detection."""

    def __init__(self, config: EngineConfig, startup_timeout: float, verbose: bool = False):
        self.config = config
        self.verbose = verbose
        self._stdout: queue.Queue[str | None] = queue.Queue()
        self._stderr: collections.deque[str] = collections.deque(maxlen=40)

        try:
            self.process = subprocess.Popen(
                config.command,
                cwd=config.cwd,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                encoding="utf-8",
                errors="replace",
                bufsize=1,
            )
        except OSError as exc:
            raise EngineError(f"could not start {config.name}: {exc}") from exc

        assert self.process.stdout is not None
        assert self.process.stderr is not None
        self._stdout_thread = threading.Thread(
            target=self._read_stdout, name=f"{config.name}-stdout", daemon=True
        )
        self._stderr_thread = threading.Thread(
            target=self._read_stderr, name=f"{config.name}-stderr", daemon=True
        )
        self._stdout_thread.start()
        self._stderr_thread.start()

        try:
            self.send("uci")
            self.wait_for(lambda line: line.strip() == "uciok", startup_timeout, "uciok")
            for name, value in config.options:
                self.send(f"setoption name {name} value {value}")
            self.sync(startup_timeout)
        except Exception:
            self.close()
            raise

    def _read_stdout(self) -> None:
        assert self.process.stdout is not None
        for line in self.process.stdout:
            line = line.rstrip("\r\n")
            if self.verbose:
                print(f"[{self.config.name}] < {line}", file=sys.stderr)
            self._stdout.put(line)
        self._stdout.put(None)

    def _read_stderr(self) -> None:
        assert self.process.stderr is not None
        for line in self.process.stderr:
            self._stderr.append(line.rstrip("\r\n"))

    def send(self, command: str) -> None:
        if self.verbose:
            print(f"[{self.config.name}] > {command}", file=sys.stderr)
        if self.process.poll() is not None:
            raise self._dead_engine_error()
        assert self.process.stdin is not None
        try:
            self.process.stdin.write(command + "\n")
            self.process.stdin.flush()
        except (BrokenPipeError, OSError) as exc:
            raise self._dead_engine_error() from exc

    def _dead_engine_error(self) -> EngineError:
        detail = "\n".join(self._stderr)
        suffix = f"; stderr tail:\n{detail}" if detail else ""
        return EngineError(
            f"{self.config.name} exited with status {self.process.poll()}{suffix}"
        )

    def _next_line(self, deadline: float) -> str:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            raise TimeoutError
        try:
            line = self._stdout.get(timeout=remaining)
        except queue.Empty as exc:
            raise TimeoutError from exc
        if line is None:
            raise self._dead_engine_error()
        return line

    def wait_for(self, predicate: Callable[[str], bool], timeout: float, description: str) -> str:
        deadline = time.monotonic() + timeout
        try:
            while True:
                line = self._next_line(deadline)
                if predicate(line):
                    return line
        except TimeoutError as exc:
            raise EngineError(f"{self.config.name} timed out waiting for {description}") from exc

    def sync(self, timeout: float) -> None:
        self.send("isready")
        self.wait_for(lambda line: line.strip() == "readyok", timeout, "readyok")

    def new_game(self, timeout: float) -> None:
        self.send("ucinewgame")
        self.sync(timeout)

    def search(self, fen: str, go_command: str, timeout: float) -> SearchResult:
        self.send(f"position fen {fen}")
        started = time.monotonic()
        self.send(go_command)
        deadline = started + timeout
        latest_score: int | None = None

        try:
            while True:
                line = self._next_line(deadline).strip()
                parsed_score = parse_uci_score(line)
                if parsed_score is not None:
                    latest_score = parsed_score
                if line.startswith("bestmove "):
                    tokens = line.split()
                    move = tokens[1] if len(tokens) > 1 else None
                    if move in {"0000", "(none)", "none"}:
                        move = None
                    elapsed_ms = round((time.monotonic() - started) * 1000)
                    return SearchResult(move, elapsed_ms, latest_score)
        except TimeoutError as exc:
            try:
                self.send("stop")
            except EngineError:
                pass
            raise EngineError(f"{self.config.name} exceeded its {timeout:.2f}s move timeout") from exc

    def close(self) -> None:
        if not hasattr(self, "process") or self.process.poll() is not None:
            return
        try:
            self.send("quit")
            self.process.wait(timeout=1.0)
        except (EngineError, subprocess.TimeoutExpired):
            self.process.terminate()
            try:
                self.process.wait(timeout=1.0)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=1.0)

    def __enter__(self) -> UciEngine:
        return self

    def __exit__(self, exc_type: object, exc: object, traceback: object) -> None:
        self.close()


def parse_uci_score(line: str) -> int | None:
    """Return a UCI info score in centipawns, mapping mates to large values."""
    if not line.startswith("info "):
        return None
    tokens = line.split()
    try:
        index = tokens.index("score")
        kind = tokens[index + 1]
        value = int(tokens[index + 2])
    except (ValueError, IndexError):
        return None
    if kind == "cp":
        return value
    if kind == "mate":
        return (1 if value >= 0 else -1) * (MATE_SCORE_CP - min(abs(value), 1000))
    return None


def parse_time_control(value: str) -> tuple[int, int]:
    match = TC_PATTERN.fullmatch(value.strip())
    if not match:
        raise argparse.ArgumentTypeError("time control must have the form BASE+INCREMENT, in seconds")
    base_ms = round(float(match.group("base")) * 1000)
    increment_ms = round(float(match.group("increment")) * 1000)
    if base_ms <= 0:
        raise argparse.ArgumentTypeError("base time must be positive")
    return base_ms, increment_ms


def elo_from_score(score: float) -> float:
    if score <= 0.0:
        return -math.inf
    if score >= 1.0:
        return math.inf
    return 400.0 * math.log10(score / (1.0 - score))


def elo_confidence_interval(match: MatchScore, z: float = 1.959963984540054) -> tuple[float, float]:
    standard_error = match.score_standard_error()
    if not math.isfinite(standard_error):
        return -math.inf, math.inf
    epsilon = 1e-12
    low_score = min(1.0 - epsilon, max(epsilon, match.score_rate - z * standard_error))
    high_score = min(1.0 - epsilon, max(epsilon, match.score_rate + z * standard_error))
    return elo_from_score(low_score), elo_from_score(high_score)


def likelihood_of_superiority(match: MatchScore) -> float:
    standard_error = match.score_standard_error()
    delta = match.score_rate - 0.5
    if not math.isfinite(standard_error):
        return 0.5
    if standard_error == 0.0:
        if delta > 0.0:
            return 1.0
        if delta < 0.0:
            return 0.0
        return 0.5
    z = delta / standard_error
    return 0.5 * (1.0 + math.erf(z / math.sqrt(2.0)))


def expected_score(elo: float) -> float:
    return 1.0 / (1.0 + 10.0 ** (-elo / 400.0))


def _term(count: int, probability: float) -> float:
    if count == 0:
        return 0.0
    if probability <= 0.0:
        return -math.inf
    return count * math.log(probability)


def constrained_trinomial_log_likelihood(match: MatchScore, elo: float) -> float:
    """Maximize W/D/L likelihood at a fixed Elo-derived expected score.

    Elo constrains W + D/2, while the draw probability is a nuisance
    parameter.  Maximizing over that one free parameter makes the SPRT usable
    across positions and time controls with different draw rates.
    """
    score = expected_score(elo)
    max_draw_probability = 2.0 * min(score, 1.0 - score)

    def log_likelihood(draw_probability: float) -> float:
        win_probability = score - 0.5 * draw_probability
        loss_probability = 1.0 - score - 0.5 * draw_probability
        return (
            _term(match.wins, win_probability)
            + _term(match.draws, draw_probability)
            + _term(match.losses, loss_probability)
        )

    low = 0.0
    high = max_draw_probability
    for _ in range(100):
        left = (2.0 * low + high) / 3.0
        right = (low + 2.0 * high) / 3.0
        if log_likelihood(left) < log_likelihood(right):
            low = left
        else:
            high = right

    candidates = (0.0, max_draw_probability, low, high, (low + high) / 2.0)
    return max(log_likelihood(value) for value in candidates)


def sprt_llr(match: MatchScore, config: SprtConfig) -> float:
    if match.games == 0:
        return 0.0
    return constrained_trinomial_log_likelihood(
        match, config.elo1
    ) - constrained_trinomial_log_likelihood(match, config.elo0)


def sprt_status(llr: float, config: SprtConfig) -> str:
    if llr >= config.upper_bound:
        return "accept H1"
    if llr <= config.lower_bound:
        return "accept H0"
    return "continue"


def parse_engine_options(raw_options: Sequence[str]) -> tuple[tuple[str, str], ...]:
    parsed: list[tuple[str, str]] = []
    for raw in raw_options:
        if "=" not in raw:
            raise HarnessError(f"engine option must be NAME=VALUE, got: {raw!r}")
        name, value = raw.split("=", 1)
        if not name.strip():
            raise HarnessError(f"engine option has an empty name: {raw!r}")
        parsed.append((name.strip(), value.strip()))
    return tuple(parsed)


def engine_config(
    name: str, command: str, cwd: str | None, raw_options: Sequence[str]
) -> EngineConfig:
    arguments = tuple(shlex.split(command))
    if not arguments:
        raise HarnessError(f"{name} command is empty")
    return EngineConfig(name, arguments, cwd, parse_engine_options(raw_options))


def require_python_chess() -> None:
    if chess is None:
        raise HarnessError(
            "python-chess is required; install it with "
            f"{shlex.quote(sys.executable)} -m pip install -r bench/requirements.txt"
        )


def load_openings(path: Path | None, opening_plies: int) -> list[str]:
    require_python_chess()
    if path is None:
        return [chess.STARTING_FEN]
    if not path.is_file():
        raise HarnessError(f"opening file does not exist: {path}")

    openings: list[str] = []
    if path.suffix.lower() == ".pgn":
        with path.open("r", encoding="utf-8", errors="replace") as stream:
            while game := chess.pgn.read_game(stream):
                board = game.board()
                for ply, move in enumerate(game.mainline_moves()):
                    if ply >= opening_plies:
                        break
                    if move not in board.legal_moves:
                        raise HarnessError(f"illegal move in opening PGN {path}: {move}")
                    board.push(move)
                if not board.is_game_over(claim_draw=True):
                    openings.append(board.fen())
    else:
        with path.open("r", encoding="utf-8", errors="replace") as stream:
            for line_number, raw_line in enumerate(stream, 1):
                line = raw_line.strip()
                if not line or line.startswith("#"):
                    continue
                try:
                    board = chess.Board(line)
                except ValueError:
                    try:
                        board, _operations = chess.Board.from_epd(line)
                    except ValueError as exc:
                        raise HarnessError(
                            f"invalid FEN/EPD at {path}:{line_number}: {line}"
                        ) from exc
                if not board.is_game_over(claim_draw=True):
                    openings.append(board.fen())

    if not openings:
        raise HarnessError(f"no playable openings found in {path}")
    return openings


def make_schedule(games: int, openings: Sequence[str], seed: int) -> list[GameSpec]:
    rng = random.Random(seed)
    order = list(range(len(openings)))
    cursor = len(order)
    schedule: list[GameSpec] = []

    def next_opening() -> int:
        nonlocal cursor
        if cursor >= len(order):
            rng.shuffle(order)
            cursor = 0
        selected = order[cursor]
        cursor += 1
        return selected

    game_number = 1
    while game_number <= games:
        opening_number = next_opening()
        schedule.append(
            GameSpec(game_number, opening_number, openings[opening_number], True)
        )
        game_number += 1
        if game_number <= games:
            schedule.append(
                GameSpec(game_number, opening_number, openings[opening_number], False)
            )
            game_number += 1
    return schedule


def result_for_winner(winner: bool | None) -> str:
    if winner is True:
        return "1-0"
    if winner is False:
        return "0-1"
    return "1/2-1/2"


def candidate_score(result: str, candidate_is_white: bool) -> float:
    if result == "1/2-1/2":
        return 0.5
    candidate_won = (result == "1-0") == candidate_is_white
    return 1.0 if candidate_won else 0.0


def pgn_for_game(
    spec: GameSpec,
    initial_board: object,
    moves: Sequence[str],
    white_name: str,
    black_name: str,
    result: str,
    termination: str,
) -> str:
    game = chess.pgn.Game()
    game.headers["Event"] = "Messier self-play"
    game.headers["Site"] = "local"
    game.headers["Date"] = dt.datetime.now().strftime("%Y.%m.%d")
    game.headers["Round"] = str(spec.number)
    game.headers["White"] = white_name
    game.headers["Black"] = black_name
    game.headers["Result"] = result
    game.headers["Termination"] = termination
    game.setup(initial_board)

    board = initial_board.copy(stack=False)
    node = game
    for move_text in moves:
        move = chess.Move.from_uci(move_text)
        node = node.add_variation(move)
        board.push(move)
    return str(game)


def play_game(
    spec: GameSpec,
    candidate: EngineConfig,
    baseline: EngineConfig,
    limit: SearchLimit,
    startup_timeout: float,
    search_timeout: float,
    clock_grace_ms: int,
    max_plies: int,
    verbose: bool,
) -> GameRecord:
    require_python_chess()
    initial_board = chess.Board(spec.fen)
    board = initial_board.copy(stack=False)
    white_config = candidate if spec.candidate_is_white else baseline
    black_config = baseline if spec.candidate_is_white else candidate
    configs = {chess.WHITE: white_config, chess.BLACK: black_config}
    engines: dict[bool, UciEngine] = {}
    moves: list[str] = []
    started = time.monotonic()
    clocks = (
        {chess.WHITE: limit.base_ms, chess.BLACK: limit.base_ms}
        if limit.mode == "clock"
        else None
    )

    def finish(winner: bool | None, reason: str) -> GameRecord:
        result = result_for_winner(winner)
        elapsed_ms = round((time.monotonic() - started) * 1000)
        final_clocks = None
        if clocks is not None:
            final_clocks = {
                "white": int(clocks[chess.WHITE]),
                "black": int(clocks[chess.BLACK]),
            }
        return GameRecord(
            number=spec.number,
            opening_number=spec.opening_number,
            initial_fen=spec.fen,
            white=white_config.name,
            black=black_config.name,
            candidate_is_white=spec.candidate_is_white,
            result=result,
            candidate_score=candidate_score(result, spec.candidate_is_white),
            termination=reason,
            plies=len(moves),
            moves=moves.copy(),
            elapsed_ms=elapsed_ms,
            final_clocks_ms=final_clocks,
            pgn=pgn_for_game(
                spec,
                initial_board,
                moves,
                white_config.name,
                black_config.name,
                result,
                reason,
            ),
        )

    try:
        for color in (chess.WHITE, chess.BLACK):
            try:
                engines[color] = UciEngine(configs[color], startup_timeout, verbose)
                engines[color].new_game(startup_timeout)
            except EngineError as exc:
                return finish(not color, f"startup failure: {exc}")

        for _ply in range(max_plies):
            outcome = board.outcome(claim_draw=True)
            if outcome is not None:
                reason = outcome.termination.name.lower().replace("_", " ")
                return finish(outcome.winner, reason)

            side = board.turn
            engine = engines[side]
            timeout = limit.move_timeout(side, clocks, search_timeout, clock_grace_ms)
            try:
                search_result = engine.search(
                    board.fen(), limit.go_command(clocks), timeout
                )
            except EngineError as exc:
                return finish(not side, f"engine failure: {exc}")

            if clocks is not None:
                clocks[side] -= search_result.elapsed_ms
                if clocks[side] < -clock_grace_ms:
                    return finish(not side, "time forfeit")

            if search_result.move is None:
                outcome = board.outcome(claim_draw=True)
                if outcome is not None:
                    reason = outcome.termination.name.lower().replace("_", " ")
                    return finish(outcome.winner, reason)
                return finish(not side, "engine returned no move")

            try:
                move = chess.Move.from_uci(search_result.move)
            except ValueError:
                return finish(not side, f"malformed move {search_result.move!r}")
            if move not in board.legal_moves:
                return finish(not side, f"illegal move {search_result.move}")

            board.push(move)
            moves.append(search_result.move)
            if clocks is not None:
                clocks[side] += limit.increment_ms

        return finish(None, "maximum ply adjudication")
    finally:
        for engine in engines.values():
            engine.close()


def format_elo(value: float) -> str:
    if value == math.inf:
        return "+inf"
    if value == -math.inf:
        return "-inf"
    return f"{value:+.1f}"


def finite_json_number(value: float) -> float | None:
    """Keep result files valid JSON when an estimate is infinite."""
    return value if math.isfinite(value) else None


def progress_line(match: MatchScore, sprt: SprtConfig) -> str:
    elo = elo_from_score(match.score_rate)
    low, high = elo_confidence_interval(match)
    los = likelihood_of_superiority(match) * 100.0
    llr = sprt_llr(match, sprt)
    return (
        f"games={match.games} W-D-L={match.wins}-{match.draws}-{match.losses} "
        f"score={100.0 * match.score_rate:.2f}% elo={format_elo(elo)} "
        f"95%CI=[{format_elo(low)}, {format_elo(high)}] LOS={los:.2f}% "
        f"LLR={llr:+.3f} [{sprt.lower_bound:+.3f}, {sprt.upper_bound:+.3f}] "
        f"{sprt_status(llr, sprt)}"
    )


def build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Paired UCI self-play with Elo estimation and trinomial SPRT."
    )
    parser.add_argument("--engine-a", required=True, help="candidate engine command")
    parser.add_argument("--engine-b", required=True, help="baseline engine command")
    parser.add_argument("--name-a", default="candidate", help="candidate display name")
    parser.add_argument("--name-b", default="baseline", help="baseline display name")
    parser.add_argument("--cwd-a", help="working directory for engine A")
    parser.add_argument("--cwd-b", help="working directory for engine B")
    parser.add_argument(
        "--option-a", action="append", default=[], metavar="NAME=VALUE",
        help="UCI option for engine A; repeat as needed",
    )
    parser.add_argument(
        "--option-b", action="append", default=[], metavar="NAME=VALUE",
        help="UCI option for engine B; repeat as needed",
    )

    limits = parser.add_mutually_exclusive_group()
    limits.add_argument("--tc", type=parse_time_control, metavar="BASE+INC", help="clock seconds")
    limits.add_argument("--movetime", type=int, metavar="MS", help="fixed milliseconds per move")
    limits.add_argument("--nodes", type=int, metavar="N", help="fixed nodes per move")
    limits.add_argument("--depth", type=int, metavar="N", help="fixed depth per move")

    parser.add_argument("--games", type=int, default=100, help="maximum games (default: 100)")
    parser.add_argument("--concurrency", type=int, default=1, help="games run in parallel")
    parser.add_argument("--openings", type=Path, help="PGN, FEN, or EPD opening file")
    parser.add_argument(
        "--opening-plies", type=int, default=8,
        help="plies imported from each PGN game (default: 8)",
    )
    parser.add_argument("--seed", type=int, default=1, help="opening shuffle seed")
    parser.add_argument("--max-plies", type=int, default=400, help="draw after this many plies")
    parser.add_argument("--startup-timeout", type=float, default=10.0)
    parser.add_argument("--search-timeout", type=float, default=60.0)
    parser.add_argument(
        "--clock-grace-ms", type=int, default=100,
        help="communication grace before a time forfeit (default: 100)",
    )
    parser.add_argument("--sprt", action="store_true", help="stop when an SPRT bound is crossed")
    parser.add_argument("--elo0", type=float, default=0.0, help="H0 Elo (default: 0)")
    parser.add_argument("--elo1", type=float, default=5.0, help="H1 Elo (default: 5)")
    parser.add_argument("--alpha", type=float, default=0.05, help="type-I error probability")
    parser.add_argument("--beta", type=float, default=0.05, help="type-II error probability")
    parser.add_argument(
        "--output-dir", type=Path,
        help="result directory (default: bench/results/<timestamp>)",
    )
    parser.add_argument("--verbose", action="store_true", help="trace UCI traffic to stderr")
    return parser


def validate_arguments(args: argparse.Namespace) -> None:
    positive_values = {
        "games": args.games,
        "concurrency": args.concurrency,
        "max-plies": args.max_plies,
        "startup-timeout": args.startup_timeout,
        "search-timeout": args.search_timeout,
    }
    for name, value in positive_values.items():
        if value <= 0:
            raise HarnessError(f"--{name} must be positive")
    if args.opening_plies < 0:
        raise HarnessError("--opening-plies cannot be negative")
    if args.clock_grace_ms < 0:
        raise HarnessError("--clock-grace-ms cannot be negative")
    if args.elo1 <= args.elo0:
        raise HarnessError("--elo1 must be greater than --elo0")
    if not 0.0 < args.alpha < 1.0 or not 0.0 < args.beta < 1.0:
        raise HarnessError("--alpha and --beta must be between zero and one")
    for name in ("nodes", "depth", "movetime"):
        value = getattr(args, name)
        if value is not None and value <= 0:
            raise HarnessError(f"--{name} must be positive")


def search_limit_from_args(args: argparse.Namespace) -> SearchLimit:
    if args.tc is not None:
        base_ms, increment_ms = args.tc
        return SearchLimit("clock", base_ms=base_ms, increment_ms=increment_ms)
    if args.movetime is not None:
        return SearchLimit("movetime", args.movetime)
    if args.depth is not None:
        return SearchLimit("depth", args.depth)
    return SearchLimit("nodes", args.nodes if args.nodes is not None else 50_000)


def default_output_directory() -> Path:
    stamp = dt.datetime.now(dt.timezone.utc).strftime("%Y%m%d-%H%M%SZ")
    return Path(__file__).resolve().parent / "results" / stamp


def write_configuration(
    path: Path,
    args: argparse.Namespace,
    candidate: EngineConfig,
    baseline: EngineConfig,
    limit: SearchLimit,
    opening_count: int,
) -> None:
    configuration = {
        "created_utc": dt.datetime.now(dt.timezone.utc).isoformat(),
        "engine_a": dataclasses.asdict(candidate),
        "engine_b": dataclasses.asdict(baseline),
        "search_limit": dataclasses.asdict(limit),
        "games": args.games,
        "concurrency": args.concurrency,
        "seed": args.seed,
        "opening_count": opening_count,
        "openings": str(args.openings) if args.openings else None,
        "opening_plies": args.opening_plies,
        "max_plies": args.max_plies,
        "sprt": {
            "enabled": args.sprt,
            "elo0": args.elo0,
            "elo1": args.elo1,
            "alpha": args.alpha,
            "beta": args.beta,
        },
    }
    path.write_text(json.dumps(configuration, indent=2) + "\n", encoding="utf-8")


def run_match(args: argparse.Namespace) -> int:
    validate_arguments(args)
    require_python_chess()
    candidate = engine_config(args.name_a, args.engine_a, args.cwd_a, args.option_a)
    baseline = engine_config(args.name_b, args.engine_b, args.cwd_b, args.option_b)
    limit = search_limit_from_args(args)
    sprt = SprtConfig(args.elo0, args.elo1, args.alpha, args.beta)
    openings = load_openings(args.openings, args.opening_plies)
    schedule = make_schedule(args.games, openings, args.seed)

    output_dir = args.output_dir or default_output_directory()
    try:
        output_dir.mkdir(parents=True, exist_ok=False)
    except FileExistsError as exc:
        raise HarnessError(
            f"output directory already exists; choose a new path: {output_dir}"
        ) from exc
    except OSError as exc:
        raise HarnessError(f"could not create output directory {output_dir}: {exc}") from exc
    write_configuration(output_dir / "config.json", args, candidate, baseline, limit, len(openings))

    print(f"candidate: {candidate.name}: {shlex.join(candidate.command)}")
    print(f"baseline:  {baseline.name}: {shlex.join(baseline.command)}")
    print(f"limit: {limit.label}; games: {args.games}; concurrency: {args.concurrency}")
    print(f"openings: {len(openings)}; output: {output_dir}")
    if args.games % 2:
        print("warning: an odd game count leaves the final opening without its color-reversed pair")

    score = MatchScore()
    jsonl_path = output_dir / "games.jsonl"
    pgn_path = output_dir / "games.pgn"
    batch_size = args.concurrency
    if args.sprt and batch_size % 2:
        batch_size += 1

    with jsonl_path.open("w", encoding="utf-8") as jsonl, pgn_path.open(
        "w", encoding="utf-8"
    ) as pgn, concurrent.futures.ThreadPoolExecutor(
        max_workers=args.concurrency
    ) as executor:
        for batch_start in range(0, len(schedule), batch_size):
            batch = schedule[batch_start : batch_start + batch_size]
            futures = [
                executor.submit(
                    play_game,
                    spec,
                    candidate,
                    baseline,
                    limit,
                    args.startup_timeout,
                    args.search_timeout,
                    args.clock_grace_ms,
                    args.max_plies,
                    args.verbose,
                )
                for spec in batch
            ]
            records = [future.result() for future in concurrent.futures.as_completed(futures)]
            records.sort(key=lambda record: record.number)

            for record in records:
                score.add(record.candidate_score)
                jsonl.write(json.dumps(record.json_dict(), sort_keys=True) + "\n")
                jsonl.flush()
                pgn.write(record.pgn.rstrip() + "\n\n")
                pgn.flush()
                print(
                    f"game {record.number}: {record.white} - {record.black} "
                    f"{record.result} ({record.termination}, {record.plies} plies)"
                )
            print(progress_line(score, sprt))

            if args.sprt and sprt_status(sprt_llr(score, sprt), sprt) != "continue":
                break

    summary = {
        "candidate": candidate.name,
        "baseline": baseline.name,
        "games": score.games,
        "wins": score.wins,
        "draws": score.draws,
        "losses": score.losses,
        "score_rate": score.score_rate,
        "elo": finite_json_number(elo_from_score(score.score_rate)),
        "elo_95_ci": [
            finite_json_number(value) for value in elo_confidence_interval(score)
        ],
        "los": likelihood_of_superiority(score),
        "llr": sprt_llr(score, sprt),
        "sprt_status": sprt_status(sprt_llr(score, sprt), sprt),
        "sprt_bounds": [sprt.lower_bound, sprt.upper_bound],
        "output_directory": str(output_dir),
    }
    (output_dir / "summary.json").write_text(
        json.dumps(summary, indent=2, allow_nan=False) + "\n", encoding="utf-8"
    )

    print("\nfinal: " + progress_line(score, sprt))
    print(f"results: {output_dir}")
    return 0


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_argument_parser()
    args = parser.parse_args(argv)
    try:
        return run_match(args)
    except HarnessError as exc:
        parser.error(str(exc))
    except KeyboardInterrupt:
        print("interrupted", file=sys.stderr)
        return 130
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
