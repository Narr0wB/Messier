import math
import unittest

from bench.selfplay import (
    MatchScore,
    SprtConfig,
    elo_from_score,
    likelihood_of_superiority,
    make_schedule,
    parse_time_control,
    parse_uci_score,
    sprt_llr,
    sprt_status,
)


class StatisticsTests(unittest.TestCase):
    def test_elo_from_score(self) -> None:
        self.assertAlmostEqual(elo_from_score(0.5), 0.0)
        self.assertAlmostEqual(elo_from_score(0.75), 400.0 * math.log10(3.0))
        self.assertEqual(elo_from_score(0.0), -math.inf)
        self.assertEqual(elo_from_score(1.0), math.inf)

    def test_los_is_symmetric_at_equal_score(self) -> None:
        score = MatchScore(wins=20, draws=60, losses=20)
        self.assertAlmostEqual(likelihood_of_superiority(score), 0.5)

    def test_sprt_prefers_positive_hypothesis_for_positive_score(self) -> None:
        score = MatchScore(wins=60, draws=0, losses=40)
        config = SprtConfig(elo0=0.0, elo1=20.0, alpha=0.05, beta=0.05)
        self.assertGreater(sprt_llr(score, config), 0.0)

    def test_sprt_bound_status(self) -> None:
        config = SprtConfig(elo0=0.0, elo1=5.0, alpha=0.05, beta=0.05)
        self.assertEqual(sprt_status(config.upper_bound, config), "accept H1")
        self.assertEqual(sprt_status(config.lower_bound, config), "accept H0")
        self.assertEqual(sprt_status(0.0, config), "continue")


class ParserAndScheduleTests(unittest.TestCase):
    def test_parse_time_control(self) -> None:
        self.assertEqual(parse_time_control("10+0.1"), (10_000, 100))

    def test_parse_uci_score(self) -> None:
        self.assertEqual(parse_uci_score("info depth 8 score cp -31 nodes 9"), -31)
        self.assertEqual(parse_uci_score("info score mate 3 pv e2e4"), 99_997)
        self.assertIsNone(parse_uci_score("bestmove e2e4"))

    def test_schedule_reverses_colors_on_same_opening(self) -> None:
        schedule = make_schedule(4, ["fen-a", "fen-b"], seed=7)
        self.assertEqual(schedule[0].fen, schedule[1].fen)
        self.assertTrue(schedule[0].candidate_is_white)
        self.assertFalse(schedule[1].candidate_is_white)
        self.assertEqual(schedule[2].fen, schedule[3].fen)


if __name__ == "__main__":
    unittest.main()
