
#ifndef EVALUATE_H
#define EVALUATE_H

#include "../movegen/position.hpp" 
#include "../movegen/types.hpp" 
#include "../search/search.hpp" 

#include <chrono>

int evaluate(const Position& position);

template <Color Us>
int corrected_eval(const Position& position) { return evaluate(position) * (Us == WHITE ? 1 : -1); };

#endif // EVALUATE_H
