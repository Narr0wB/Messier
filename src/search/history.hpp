
#ifndef HISTORY_HPP
#define HISTORY_HPP

#include "movegen/move.hpp"
#include "search/parameters.hpp"

#define MAX_TABLE MAX_DEPTH + 1
#define MAX_HISTORY 8000

struct QuietHistory {
    int board[64][64][2];

    template <Color C> inline void update_history(const Move& m, int bonus) 
    {
        int clamped_bonus = std::clamp(bonus, -MAX_HISTORY, MAX_HISTORY);
        board[m.from()][m.to()][C]
            += clamped_bonus - board[m.from()][m.to()][C] * std::abs(clamped_bonus) / MAX_HISTORY;
    }
};

struct KillerHistory {
    Move moves[MAX_TABLE][2];
};

struct ButterflyHistory {

};

struct CaptureHistory {

};

struct ContinuationHistory {

};

struct CounterMovesHistory {

};

#endif // HISTORY_HPP