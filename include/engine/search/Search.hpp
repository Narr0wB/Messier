
#ifndef SEARCH_H 
#define SEARCH_H

#include <iostream>

#include <engine/movegen/Position.hpp>
#include <engine/Misc.hpp>
#include <engine/movegen/Types.hpp>
#include <engine/search/Transposition.hpp>
#include <engine/search/Evaluate.hpp>
#include <engine/Log.hpp>

#define INF 5000000
#define TIME_LIMIT 15 * 1000ms

#define MAX_DEPTH 20
#define MAX_TABLE 21

struct RepetitionTable {
    uint64_t hashes[2];
    int count = 0;
};

struct SearchInfo {
    uint64_t search_start_time = 0;
    uint64_t search_end_time   = 0;
    bool timeset = false;
    bool movetimeset = false;
    bool nodeset = false;
    uint64_t nodeslimit;
    uint64_t nodes;
    uint32_t movestogo;
    int depth;
    int quiescence_depth;
};

struct SearchData {
    Move pv_table[MAX_TABLE][MAX_TABLE];
    int pv_table_len[MAX_TABLE];

    Move killer_moves[2][MAX_TABLE];
    int history_moves[64][64];
};

struct SearchContext {
    std::atomic<bool> stop_flag;

    Position board;
    std::shared_ptr<TranspositionTable> table;
    RepetitionTable repetition;

    SearchInfo info;
    SearchData data;

    // debug
    uint32_t reduced_nodes;
};

namespace Search {

int mvv_lva(const Move &m_, const Position &p_);

#define MAX_MOVE_SCORE 1000000
int score_move(const Move& m_, const std::shared_ptr<SearchContext> ctx, int ply);

// Order moves using context
template <Color Us>
struct move_sorting_criterion {
    const std::shared_ptr<SearchContext> _ctx;
    int _ply;

    move_sorting_criterion(const std::shared_ptr<SearchContext> c, int p) : _ctx(c), _ply(p) {}; 

    bool operator() (const Move& a, const Move& b) {
        return score_move(a, _ctx, _ply) > score_move(b, _ctx, _ply);
    }
};

template <Color Us>
void order_move_list(MoveList<Us>& m, const std::shared_ptr<SearchContext> ctx, int ply) {
    std::stable_sort(m.begin(), m.end(), move_sorting_criterion<Us>(ctx, ply));
}

template <Color C>
int Quiescence(std::shared_ptr<SearchContext> ctx, int Aalpha, int Bbeta, int depth) {
    ctx->info.nodes++;

    int score = Evaluate(ctx->board) * (C == WHITE ? 1 : -1);
    int ply = ctx->info.quiescence_depth - depth;

    if ((ctx->stop_flag) ||
        (ctx->info.timeset && GetTimeMS() >= ctx->info.search_end_time) || 
        (ctx->info.nodeset && ctx->info.nodes > ctx->info.nodeslimit)) {
        return 0;
    }

    if (score >= Bbeta) {
        return Bbeta;
    }

    if (score > Aalpha) {
        Aalpha = score;
    }

    if (depth == 0) {
        return Aalpha;
    }

    MoveList<C> mL(ctx->board);

    order_move_list<C>(mL, ctx, ply);

    for (const Move& m : mL) {
        if (m.flags() != MoveFlags::CAPTURE) continue;

        ctx->board.play<C>(m);

        score = -Quiescence<~C>(ctx, -Bbeta, -Aalpha, depth - 1);

        ctx->board.undo<C>(m);

        if (score > Aalpha) {
            Aalpha = score;
        }

        if (Aalpha >= Bbeta) {
            return Bbeta;
        }
    }

    return Aalpha;
}

template <Color C>
int negamax(std::shared_ptr<SearchContext> ctx, int Aalpha, int Bbeta, int depth) {
    ctx->info.nodes++;

    int ply = ctx->info.depth - depth;
    int move_count = 0;
    int score = 0;
    int found_pv = 0;
    ctx->data.pv_table_len[ply] = ply;

    int repetitions = 0;
    for (int i = ctx->board.ply() - 2; i >= 0; i -= 2) {
        // If we are above the root ply, and we find a position that has been played before during our search
        // then that means that most likely this will be repeated, leading to a three fold repetition draw
        // or if we just find 3 repetitions (including from before root positions) (we have 3 repetitions
        // because we have found 2 + the one we are currently analyzing)
          
        if (ctx->board.get_hash() == ctx->board.history[i].hash &&
            (i > ctx->board.ply() - ply || ++repetitions == 2)) {
            return 0;
        }
    }

    if (depth <= 0) {
        int score = Quiescence<C>(ctx, Aalpha, Bbeta, ctx->info.quiescence_depth);

        return score;
    }

    // If out of time or hit any other constraints then exit the search
    if ((ctx->stop_flag) ||
        (ctx->info.timeset && GetTimeMS() >= ctx->info.search_end_time) || 
        (ctx->info.nodeset && ctx->info.nodes > ctx->info.nodeslimit)) {
        return 0;
    }

    Transposition tt_hit = ctx->table->probe_hash(ctx->board.get_hash(), Aalpha, Bbeta, depth);
    if (tt_hit.flags != FLAG_EMPTY && ply >= 2) {
        return tt_hit.score;
    }

    Transposition node_ = Transposition{FLAG_ALPHA, ctx->board.get_hash(), (int8_t)depth, NO_SCORE, NO_MOVE};
    
    MoveList<C> mL(ctx->board);

    if (mL.size() == 0) {
        if (ctx->board.in_check<C>()) {
            // If checkmate
            return SHRT_MIN + ply;
        }
        else {
            // If stalemate
            return 0;
        }
    } 

    order_move_list(mL, ctx, ply);

    for (const Move& m : mL) {
        move_count++;
        ctx->board.play<C>(m);

        if (found_pv) {
            score = -negamax<~C>(ctx, -Aalpha - 1, -Aalpha, depth - 1);

            if (score > Aalpha && score < Bbeta) {
                score = -negamax<~C>(ctx, -Bbeta, -Aalpha, depth - 1);
            }
        }
        else {
            if (move_count == 1) score = -negamax<~C>(ctx, -Bbeta, -Aalpha, depth - 1);

            // Late move reduction 
            else if (ply > 2 && move_count > 4 && m.flags() != MoveFlags::CAPTURE && !ctx->board.in_check<C>()) {
                // If the conditions are met then we do a search at reduced depth with a reduced window (two fold deeper)
                score = -negamax<~C>(ctx, -Aalpha - 1, -Aalpha, depth - 1 - 2);

                if (score > Aalpha) {
                    score = -negamax<~C>(ctx, -Aalpha - 1, -Aalpha, depth - 1);

                    if (score > Aalpha && score < Bbeta) {
                        // Do full depth and full window
                        score = -negamax<~C>(ctx, -Bbeta, -Aalpha, depth - 1);
                    }
                }

                ctx->reduced_nodes++;
            }
            else {
                // Do reduced window
                score = -negamax<~C>(ctx, -Aalpha - 1, -Aalpha, depth - 1);

                if (score > Aalpha && score < Bbeta) {
                    score = -negamax<~C>(ctx, -Bbeta, -Aalpha, depth - 1);
                }
            }
        }  
        
        ctx->board.undo<C>(m);

        // We have found a move that is better than the current alpha
        if (score > Aalpha) {
            Aalpha = score;
            found_pv = 1;

            // Update the main PV line
            ctx->data.pv_table[ply][ply] = m;
            for (int i = ply + 1; i < ctx->data.pv_table_len[ply + 1]; ++i) {
                ctx->data.pv_table[ply][i] = ctx->data.pv_table[ply + 1][i];
            }
            ctx->data.pv_table_len[ply] = ctx->data.pv_table_len[ply + 1];
            
            // Rank history moves
            if (m.flags() == MoveFlags::QUIET) ctx->data.history_moves[m.from()][m.to()] += depth;

            node_.flags = FLAG_EXACT;
            node_.best = m;
        }

        // Fail High Node, i.e. we have found a move that is better than what our opponent is guaranteed to take, which means
        // that this move will not be taken, though it is useful to keep track of these moves
        if (Aalpha >= Bbeta) {
            if (m.flags() != MoveFlags::CAPTURE) {
                ctx->data.killer_moves[1][ply] = ctx->data.killer_moves[0][ply];
                ctx->data.killer_moves[0][ply] = m;
            }

            node_.flags = FLAG_BETA;
            node_.score = Bbeta;

            ctx->table->push_position(node_);

            return Bbeta;
        }

        if ((ctx->stop_flag) ||
            (ctx->info.timeset && GetTimeMS() >= ctx->info.search_end_time) || 
            (ctx->info.nodeset && ctx->info.nodes > ctx->info.nodeslimit)) {
            return 0;
        }
    }

    node_.score = Aalpha;
    ctx->table->push_position(node_);

    // Fail Low Node - No better move was found
    return Aalpha;
}

template <Color C>
void Search(std::shared_ptr<SearchContext> ctx) {
    int alpha = -INF;
    int beta = INF;

    int max_depth = ctx->info.depth;

    int current_depth = 0;
    Move bestMove = NO_MOVE;
    for (; current_depth <= max_depth; ++current_depth) {
        // Set the context's search depth to the current depth in the iterative deepening process
        ctx->info.depth = current_depth;
        
        auto last_search_nodes = ctx->info.nodes;
        auto start_current_search = GetTimeMS();

        // Score the current position
        int score = negamax<C>(ctx, alpha, beta, current_depth) * (C == WHITE ? 1 : -1);

        auto end_current_search = GetTimeMS();

        // If we are out of time or over the limit for nodes (if there is one) then break
        if ((ctx->stop_flag) ||
            (ctx->info.timeset && GetTimeMS() >= ctx->info.search_end_time) || 
            (ctx->info.nodeset && ctx->info.nodes > ctx->info.nodeslimit)) {
            break;
        }
        
        uint32_t nps = (float)(ctx->info.nodes - last_search_nodes) / ((end_current_search - start_current_search) / 1000.0f);   

        std::cout << "info depth " << current_depth << " score cp " << score << " nodes " << ctx->info.nodes << " nps " << nps << " tthits " << (ctx->table->hits) << " pvlen " << ctx->data.pv_table_len[0] << " pv ";
        for (int j = 0; j < ctx->data.pv_table_len[0]; j++) {
            std::cout << ctx->data.pv_table[0][j] << " ";
        }
        std::cout << std::endl;

        bestMove = ctx->data.pv_table[0][0];

        ctx->table->hits = 0;
    }

    ctx->info.nodes = 0;

    // Print the best move found
    std::cout << "bestmove ";
    std::cout << bestMove;
    std::cout << std::endl;
}

} // namespace Search

#endif // SEARCH_H
