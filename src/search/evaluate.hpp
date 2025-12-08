
#ifndef EVALUATE_H
#define EVALUATE_H

#include "../movegen/position.hpp" 
#include "../movegen/types.hpp" 

#include <chrono>

using namespace std::chrono_literals;

const int piece_value[NPIECE_TYPES + 1] = {100, 300, 320, 500, 900, 20000, 0};
int Evaluate(Position& position);

// Return a corrected evaluation relative to the side
template <Color Us>
int corrected_eval(Position& position) { return Evaluate(position) * (Us == WHITE ? 1 : -1); };

int mvv_lva(const Move &m_, const Position &p_);

#define MAX_MOVE_SCORE 20000
int score_move(const Move& m_, const std::shared_ptr<SearchContext>& ctx, int ply, Move tt_move);

#endif // EVALUATE_H
