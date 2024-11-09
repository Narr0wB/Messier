
#ifndef EVALUATE_H
#define EVALUATE_H

#include <engine/movegen/Position.hpp>
#include <engine/movegen/Types.hpp>

#include <engine/search/Transposition.hpp>
#include <engine/Log.hpp>

using namespace std::chrono_literals;

const int piece_value[NPIECE_TYPES + 1] = {100, 300, 320, 500, 900, 20000, 0};
int Evaluate(Position& position);

#endif // EVALUATE_H
