
#ifndef EVALUATE_H
#define EVALUATE_H

#include <engine/movegen/Position.hpp>
#include <engine/movegen/Types.hpp>

#include <engine/search/Transposition.hpp>
#include <engine/Log.hpp>

using namespace std::chrono_literals;

const int piece_value[NPIECE_TYPES + 1] = {100, 300, 320, 500, 900, 20000, 0};
int Evaluate(Position& position);

// Return a corrected evaluation relative to the side
template <Color Us>
int corrected_eval(Position& position) { return Evaluate(position) * (Us == WHITE ? 1 : -1); };

#endif // EVALUATE_H
