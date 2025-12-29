
#ifndef EVALUATE_H
#define EVALUATE_H

#include "../movegen/position.hpp" 
#include "../movegen/types.hpp" 
#include "../search/search.hpp" 

#include <chrono>

#define MAX_MOVE_SCORE 20000

const int piece_value[NPIECE_TYPES + 1] = {100, 300, 320, 500, 900, 20000, 0};

int evaluate(Position& position);

template <Color Us>
int corrected_eval(Position& position) { return evaluate(position) * (Us == WHITE ? 1 : -1); };

int mvv_lva(const Move &m_, const Position &p_);

int score_move(const Move& m_, const Search::SearchContext& ctx, int ply, Move tt_move);

#endif // EVALUATE_H
