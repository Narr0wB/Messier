
#ifndef SEARCHTYPES_HPP
#define SEARCHTYPES_HPP

#include <engine/movegen/Types.hpp>
#include <engine/movegen/Position.hpp>
#include <engine/search/Transposition.hpp>

#include <memory>

#define MAX_DEPTH 20
#define MAX_TABLE MAX_DEPTH + 1

#define MATE_SCORE UINT16_MAX 

struct SearchConfig {
    uint64_t search_start_time = 0;
    uint64_t search_end_time   = 0;

    bool timeset = false;
    bool movetimeset = false;
    bool nodeset = false;

    uint64_t nodeslimit;
    uint32_t movestogo;

    int depth;
    int quiescence_depth;
};

struct SearchInfo {
    uint64_t nodes;
    uint64_t qnodes;
    uint64_t reduced_nodes;
};

struct SearchStack {
    int ply;
    int static_eval;
    int move_count;
    bool tt_hit;
};

struct SearchContext {
    Position board;

    TranspositionTable ttable;

    Move pv_table[MAX_TABLE][MAX_TABLE];
    int pv_table_len[MAX_TABLE];

    Move killer_moves[MAX_TABLE][2];
    int history_moves[64][64];

    SearchInfo info;
};

#endif // SEARCHTYPES_HPP