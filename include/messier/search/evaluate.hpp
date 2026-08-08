
#ifndef EVALUATE_H
#define EVALUATE_H

#include <messier/movegen/position.hpp>
#include <messier/movegen/types.hpp>
#include <messier/search/search.hpp>

#include <chrono>

int evaluate(const Position& position);

template <Color Us>
int corrected_eval(const Position& position) { return evaluate(position) * (Us == WHITE ? 1 : -1); };

#endif // EVALUATE_H
