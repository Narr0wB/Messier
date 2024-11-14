
#ifndef SEARCH_H 
#define SEARCH_H

#include <cmath>
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
    std::atomic<bool> stop;

    Position board;
    std::shared_ptr<TranspositionTable> table;

    SearchInfo info;
    SearchData data;

    // debug
    uint32_t reduced_nodes = 0;
    uint64_t q_nodes = 0;
};

namespace Search {
    int mvv_lva(const Move &m_, const Position &p_);

    template <Color Us>
    int SEE(Position& pos, Square to) {
        int value = 0;
        Bitboard occupied = pos.all_pieces<WHITE>() | pos.all_pieces<BLACK>();

        int pt;
        Square from = NO_SQUARE;
        Bitboard attackers;
        for (pt = PAWN; pt < KING; pt++) {
            if ( (attackers = pos.attacker_from<Us>((PieceType)pt, to, occupied)) ) {
                from = pop_lsb(&attackers);
                break;
            }
        }

        if (from != NO_SQUARE) {
            int captured = pos.at(to) == NO_PIECE ? 6 : type_of(pos.at(to));
            Move see_move(from, to, captured == 6 ? MoveFlags::QUIET : MoveFlags::CAPTURE);

            pos.play<Us>(see_move);

            value = piece_value[captured] - SEE<~Us>(pos, to);    

            pos.undo<Us>(see_move);
        }
        
        return value;
    }

    #define MAX_MOVE_SCORE 1000000
    int score_move(const Move& m_, const std::shared_ptr<SearchContext>& ctx, int ply);

    // Order moves using context
    template <Color Us>
    struct move_sorting_criterion {
        const std::shared_ptr<SearchContext>& _ctx;
        int _ply;

        move_sorting_criterion(const std::shared_ptr<SearchContext>& c, int p) : _ctx(c), _ply(p) {}; 

        bool operator() (const Move& a, const Move& b) {
            // Since the std::stable_sort function actually sorts elements in a list in ascending order by checking if a < b is true,  
            // we have to flip the comparison in order to have a list ordered in descending order 
            return score_move(a, _ctx, _ply) > score_move(b, _ctx, _ply);
        }
    };

    template <Color Us>
    void order_move_list(MoveList<Us>& m, const std::shared_ptr<SearchContext> ctx, int ply) {
        // This function sorts the movelist in descending order
        std::stable_sort(m.begin(), m.end(), move_sorting_criterion<Us>(ctx, ply));
    }

    template <Color C>
    int Quiescence(std::shared_ptr<SearchContext>& ctx, int Aalpha, int Bbeta, int depth) {
        ctx->info.nodes++;
        ctx->q_nodes++;

        if ((ctx->stop) ||
            (ctx->info.timeset && GetTimeMS() >= ctx->info.search_end_time) || 
            (ctx->info.nodeset && ctx->info.nodes > ctx->info.nodeslimit)) {
            return 0;
        }

        int score = Evaluate(ctx->board) * (C == WHITE ? 1 : -1);
        int best_score = -INF;
        int ply = ctx->info.quiescence_depth - depth;

        if (score >= Bbeta) {
            return Bbeta;
        }

        if (score > best_score) {
            best_score = score;

            if (best_score > Aalpha) {
                Aalpha = best_score;
            }
        }
        
        if (depth == 0) {
            return best_score;
        }

        MoveList<C> mL(ctx->board);
        order_move_list<C>(mL, ctx, ply);

        for (const Move& m : mL) {
            if (m.flags() != MoveFlags::CAPTURE) continue;

            int see = SEE<C>(ctx->board, m.to());
            if (see < 0)
                continue;

            ctx->board.play<C>(m);

            score = -Quiescence<~C>(ctx, -Bbeta, -Aalpha, depth - 1);

            ctx->board.undo<C>(m);

            if (score > best_score) {
                best_score = score;

                if (best_score > Aalpha) {
                    Aalpha = best_score;
                }
            }

            if (best_score >= Bbeta) {
                return Bbeta;
            }
        }

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

        return Aalpha;
    }

    template <Color C>
    int negamax(std::shared_ptr<SearchContext>& ctx, int Aalpha, int Bbeta, int depth) {
        ctx->info.nodes++;

        int ply = ctx->info.depth - depth;
        int move_count = 0;
        int score = 0;
        int best_score = -INF;
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
            return Quiescence<C>(ctx, Aalpha, Bbeta, ctx->info.quiescence_depth);
        }

        // If out of time or hit any other constraints then exit the search
        if ((ctx->stop) ||
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
                else if (depth >= 3 && move_count > 3 && m.flags() != MoveFlags::CAPTURE && !ctx->board.in_check<C>()) {
                    // If the conditions are met then we do a search at reduced depth with a reduced window (two fold deeper)
                    int reduction = std::max(1, depth - 1 - (move_count - 1));
                    score = -negamax<~C>(ctx, -Aalpha - 1, -Aalpha, reduction);

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

            if (score > best_score) {
                best_score = score;
                // We have found a move that is better than the current alpha
                if (best_score > Aalpha) {
                    Aalpha = best_score;
                    found_pv = 1;
                    
                    // Triangular transposition tables
                    // 
                    // For each ply we have a pv_array 
                    // 
                    // 0: m0 m1 m2 m3 m4
                    // 1: N  m1 m2 m3 m4
                    // 2: N  N  m2 m3 m4
                    // 3: N  N  N  m3 m4
                    // 4: N  N  N  N  m4 
                    
                    // and each ply copies the moves at the next ply 
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
            }

            // Fail High Node, i.e. we have found a move that is better than what our opponent is guaranteed to take, which means
            // that this move will not be taken, though it is useful to keep track of these moves
            if (best_score >= Bbeta) {
                if (m.flags() != MoveFlags::CAPTURE) {
                    ctx->data.killer_moves[1][ply] = ctx->data.killer_moves[0][ply];
                    ctx->data.killer_moves[0][ply] = m;
                }

                node_.flags = FLAG_BETA;
                node_.score = best_score;

                ctx->table->push_position(node_);

                return Bbeta;
            }

            if ((ctx->stop) ||
                (ctx->info.timeset && GetTimeMS() >= ctx->info.search_end_time) || 
                (ctx->info.nodeset && ctx->info.nodes > ctx->info.nodeslimit)) {
                return 0;
            }
        }

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

        node_.score = best_score;
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
        int as_window = 50;
        Move bestMove = NO_MOVE;

        for (; current_depth <= max_depth; ++current_depth) {
            // Set the context's search depth to the current depth in the iterative deepening process
            ctx->info.depth = current_depth;
            
            auto last_search_nodes = ctx->info.nodes;
            auto start_current_search = GetTimeMS();

            int score = negamax<C>(ctx, alpha, beta, current_depth);

            auto end_current_search = GetTimeMS();

            // If we fall out of our aspiration window then reset the alpha and beta parameters;
            // if ((score <= alpha) || (score >= beta)) {
            //     alpha = -INF;
            //     beta = INF;
            // } else {
            //     // Update aspiration window
            //     alpha = score - as_window;
            //     beta = score + as_window; 
            // }

            // If we are out of time or over the limit for nodes (if there is one) then break
            if ((ctx->stop) ||
                (ctx->info.timeset && GetTimeMS() >= ctx->info.search_end_time) || 
                (ctx->info.nodeset && ctx->info.nodes > ctx->info.nodeslimit)) {
                break;
            }
            
            uint32_t nps = (float)(ctx->info.nodes - last_search_nodes) / ((end_current_search - start_current_search) / 1000.0f);   

            std::cout << "info depth " << current_depth << " score cp " << score * (C == WHITE ? 1 : -1) << " nodes " << ctx->info.nodes << " nps " << nps << " tthits " << ctx->table->hits << " qnodes " << ctx->q_nodes << " BF " << std::pow(ctx->info.nodes, 1.0f / current_depth) << " pvlen " << ctx->data.pv_table_len[0] << " pv ";
            for (int j = 0; j < ctx->data.pv_table_len[0]; j++) {
                std::cout << ctx->data.pv_table[0][j] << " ";
            }
            std::cout << std::endl;

            bestMove = ctx->data.pv_table[0][0];

            ctx->table->hits = 0;
            ctx->q_nodes = 0;
            ctx->info.nodes = 0;
        }

        ctx->reduced_nodes = 0;

        // Print the best move found
        std::cout << "bestmove ";
        std::cout << bestMove;
        std::cout << std::endl;
    }
} // namespace Search

#endif // SEARCH_H
