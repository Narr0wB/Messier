
#ifndef SEARCHTYPES_HPP
#define SEARCHTYPES_HPP

#include "../movegen/types.hpp"
#include "../movegen/position.hpp"

#include <cstdint>


struct SearchContext {
    Position board;

    TranspositionTable ttable;


    SearchInfo info;
};

#endif // SEARCHTYPES_HPP