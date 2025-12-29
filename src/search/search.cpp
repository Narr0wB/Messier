
#include "search.hpp" 
#include "evaluate.hpp"
#include "../misc.hpp"

#include <atomic>
#include <algorithm>

namespace Search {
    void Worker::idle_loop() {
        while (true) {
            std::unique_lock<std::mutex> lock(m_mutex);

            // Stop waiting on any state that is not idle
            m_cv.wait(lock, [&]{ return (m_state != WorkerState::IDLE); });
            lock.unlock();

            switch (m_state) {
                case WorkerState::SEARCHING: iterative_deepening(); break;
                case WorkerState::DEAD: return; break;
            }

            lock.lock();
            m_state = WorkerState::IDLE;
            m_stop = false;
        }
    }

    void Worker::run(Position& root, const SearchConfig& cfg) {
        // Stop any previous searches
        stop();

        std::unique_lock<std::mutex> lock(m_mutex);
        m_cfg = cfg;
        m_root = root;
        m_state = WorkerState::SEARCHING;
        m_cv.notify_one();
    }

    void Worker::clear()
    {
        m_ctx  = {0};
        m_cfg  = {0};
        m_info = {0};
    }

    void Worker::stop() {
        if (m_state == WorkerState::SEARCHING) m_stop = true;
    }

    void Worker::kill() {
        stop();
        std::unique_lock<std::mutex> lock(m_mutex);
        m_state = WorkerState::DEAD;
        m_cv.notify_one();
        m_thread.join();
    }

    WorkerState Worker::get_state() {
        return m_state;
    }

    template <Color Us>
    int SEE(Position& pos, Square to) 
    {
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
   
    int mate_in(int ply) {
        return MATE_SCORE - ply;
    }

    int mated_in(int ply) {
        return -MATE_SCORE + ply;
    }

    template <Color C, bool PVnode>
    int Worker::quiescence(Position& root, int Aalpha, int Bbeta, int depth) 
    {
        m_info.nodes++;
        m_info.qnodes++;

        if ((m_stop) ||
            (m_cfg.timeset && (time_ms() >= m_cfg.search_end_time)) || 
            (m_cfg.nodeset && (ctx.info.nodes > m_cfg.nodeslimit))) 
        {
            return 0;
        }
        
        auto [tt_hit, tte] = m_tt.probe(pos.get_hash());
        Move tt_move = tte.move;
        int tt_score = tte.score;
        uint8_t tt_bound = tte.flags;
        int8_t tt_depth = tte.depth;

        // If we are not in a pv node, and we got a useful score from the TT, return early
        // NOTE: we do not check if the tte depth is greater (or equal) than the current depth because in quiescence any depth IS greater or equal than 0 
        if (!PVnode 
            && tt_hit 
            && ( (tt_bound == FLAG_ALPHA && tt_score <= Aalpha)
            ||   (tt_bound == FLAG_BETA  && tt_score >= Bbeta)
            ||   (tt_bound == FLAG_EXACT))) 
        {
            return tt_score;
        }

        int static_eval = corrected_eval<C>(pos); 
        int score = static_eval;
        int best_score = -INF;
        int ply = cfg.quiescence_depth - depth;

        if (score >= Bbeta) return score;

        if (score > best_score) {
            best_score = score;
            if (best_score > Aalpha) Aalpha = best_score;
        }
        
        if (depth == 0) return best_score;

        Transposition current_node = {FLAG_ALPHA, pos.get_hash(), 0, best_score, static_eval, NO_MOVE}; 
        MoveList<C> mL(pos, ctx, ply, tt_move);
        order_move_list(mL, ctx, ply, tt_move);

        for (const Move& m : mL) {
            if (isQuiet(m)) continue;
            
            int see = SEE<C>(pos, m.to());
            if (see < 0)
                continue;

            ctx->board.play<C>(m);

            score = -quiescence<~C, PVnode>(ctx, -Bbeta, -Aalpha, depth - 1);

            ctx->board.undo<C>(m);

            if (score > best_score) {
                best_score = score;
                current_node.score = best_score;
                current_node.move = m;

                if (best_score > Aalpha) {
                    Aalpha = best_score;

                    current_node.flags = FLAG_EXACT;

                    if (best_score >= Bbeta) {
                        current_node.flags = FLAG_BETA;
                        return best_score;
                    }
                }
            }
        }

        if (mL.size() == 0) {
            if (ctx->board.in_check<C>()) 
                return mated_in(ply);
            else 
                return 0;
        }

        return best_score;
    }

    template <Color C, bool PVnode>
    int Worker::search(Position& pos, SearchStack *ss, int Aalpha, int Bbeta, int depth) 
    {
        m_info.nodes++;

        int ply = ss->ply;
        int move_count = 0;
        int score = 0;
        int static_eval = 0;
        int best_score = -INF;
        m_ctx.pv_table_len[ply] = ply;

        // If depth is below zero, dip into quiescence search
        if (depth <= 0) {
            return quiescence<C, PVnode>(pos, Aalpha, Bbeta, m_cfg.quiescence_depth);
        }

        assert(PVnode || (Aalpha == Bbeta - 1));
        assert(0 < depth && depth < MAX_DEPTH + 1);

        // If out of time or hit any other constraints then exit the search
        if ((m_stop) ||
            (cfg.timeset && (time_ms() >= cfg.search_end_time)) || 
            (cfg.nodeset && (ctx.info.nodes > cfg.nodeslimit))) 
        {
            return 0;
        }

        if (ply != 0) {
            int repetitions = 0;
            for (int i = pos.ply() - 2; i >= 0; i -= 2) {
                // If we are at a non-zero ply, and we find that the current position has been played before during the current search
                // then that means that most likely this exact position will be repeated once more down the line, leading to a three fold repetition draw.
                // Or, having exhausted the positions explored in the current variation, if we just find 2 more repetitions in the general board history  
                // (including from before root positions) then we have a three fold repetition draw. 
                
                if (pos.get_hash() == pos.history[i].hash &&
                    (i > pos.ply() - ply || ++repetitions == 2)) {
                    return 0;
                }
            }

            // Mate distance pruning 
            int alpha = std::max(mated_in(ply), Aalpha);
            int beta  = std::min(mate_in(ply + 1), Bbeta);
            if (alpha >= beta)
                return alpha;
        }
        
        // Futility pruning, if at frontier nodes we realize that the static evaluation of our position, even after adding the value of a queen, is still under alpha then 
        // prune this node by returning the static evaluation  
        // if (depth == 1 && !ctx->board.in_check<C>() && static_eval + piece_value[QUEEN] < Aalpha) {
        //     return static_eval + piece_value[QUEEN];   
        // }

        auto [tt_hit, tte] = m_tt.probe(pos.get_hash());
        Move tt_move = tte.move;
        int tt_score = tte.score;
        uint8_t tt_bound = tte.flags;
        int8_t tt_depth = tte.depth;

        // If we are not in a pv node, and we got a useful score from the TT, return early
        if (!PVnode 
            && tt_hit 
            && tt_depth >= depth
            && ( (tt_bound == FLAG_ALPHA && tt_score <= Aalpha)
            ||   (tt_bound == FLAG_BETA  && tt_score >= Bbeta)
            ||   (tt_bound == FLAG_EXACT))) 
        {
            return tt_score;
        }
        
        if (pos.in_check<C>()) 
            static_eval = ss->static_eval = 0;
        else if (tt_hit) 
            static_eval = ss->static_eval = tte.eval; 
        else 
            static_eval = ss->static_eval = corrected_eval<C>(pos); 

        Transposition node_ = Transposition{FLAG_ALPHA, pos.get_hash(), (int8_t)depth, static_eval, NO_SCORE, NO_MOVE};
        
        // Generate moves and order them
        MoveList<C> mL(pos, ctx, ply, tt_move);
        order_move_list(mL, ctx, ply, tt_move);

        for (const Move& m : mL) {
            move_count++;
            pos.play<C>(m);
            
            // Late move reduction 
            if (depth >= 3 
                && move_count > 3 
                && isQuiet(m) 
                && (!pos.in_check<C>() && !pos.in_check<~C>())
                && !PVnode) 
            {
                // If the conditions are met then we do a search at reduced depth with a reduced window (two fold deeper)
                int reduced = std::max(1, depth - (move_count - 2) - 1);
                score = -search<~C, false>(pos, ss + 1, -Aalpha - 1, -Aalpha, reduced);

                if (score > Aalpha) {
                    score = -negamax<~C, false>(pos, ss + 1, -Aalpha - 1, -Aalpha, depth - 1);
                }

                m_info.qnodes++;
            }

            // If we cannot reduce or we are not in a PV node, then search at full depth but with a reduced window
            else if (move_count > 1 || !PVnode) {
                score = -search<~C, false>(pos, ss + 1, -Aalpha - 1, -Aalpha, depth - 1);
            }

            if (PVnode && (move_count == 1 || score > Aalpha && (ply == 0 || score < Bbeta))) {
                score = -search<~C, true>(pos, ss + 1, -Bbeta, -Aalpha, depth - 1);
            }

            pos.undo<C>(m);

            if (score > best_score) {
                best_score = score;
                node_.move = m;
                node_.score = best_score;

                // We have found a move that is better than the current alpha
                if (best_score > Aalpha) {
                    Aalpha = best_score;
                    
                    // Triangular transposition tables
                    // ply 0: m0 m1 m2 m3 m4
                    // ply 1: N  m1 m2 m3 m4
                    // ply 2: N  N  m2 m3 m4
                    // ply 3: N  N  N  m3 m4
                    // ply 4: N  N  N  N  m4 
                    
                    if (PVnode) {
                        m_ctx.pv_table[ply][ply] = m;
                        for (int i = ply + 1; i < m_ctx.pv_table_len[ply + 1]; ++i) {
                            m_ctx.pv_table[ply][i] = m_ctx.pv_table[ply + 1][i];
                        }
                        m_ctx.pv_table_len[ply] = m_ctx.pv_table_len[ply + 1];
                    }

                    // Rank history moves
                    if (isQuiet(m)) 
                        m_ctx.history_moves[m.from()][m.to()] += depth;

                    node_.flags = FLAG_EXACT;

                    // Fail High Node, i.e. we have found a move that is better than what our opponent is guaranteed to take, which means
                    // that this move will not be taken, though it is useful to keep track of these moves
                    if (best_score >= Bbeta) {
                        if (isQuiet(m)) {
                            m_ctx.killer_moves[ply][1] = m_ctx.killer_moves[ply][0];
                            m_ctx.killer_moves[ply][0] = m;
                        }

                        node_.flags = FLAG_BETA;
                        m_tt.push(node_);

                        return best_score;
                    }
                }
            }

            if ((m_stop) ||
                (m_cfg.timeset && (time_ms() >= m_cfg.search_end_time)) || 
                (m_cfg.nodeset && (ctx.info.nodes > m_cfg.nodeslimit))) 
            {
                return 0;
            }
        }

        if (mL.size() == 0) {
            if (pos.in_check<C>()) {
                // If checkmate
                return mated_in(ply);
            }
            else {
                // If stalemate
                return 0;
            }
        } 

        m_tt.push(node_);

        // Fail Low Node - No better move was found
        return best_score;
    }

    void Worker::iterative_deepening() 
    {
        int alpha = -INFTY;
        int beta = INFTY;
        int max_depth = m_cfg.max_depth;
        int current_depth = 1;
        int score_avg = 0;

        Color to_play = m_root.turn();
        Move best_move = NO_MOVE;
        SearchStack ss[MAX_DEPTH + 1];

        for (; current_depth <= max_depth; ++current_depth) {
            int root_depth = current_depth;
            int depth      = root_depth;
            int alpha      = -INFTY;
            int beta       = INFTY;
            int score      = 0;
            int delta      = 12;
            int score      = 0;

            // Wipe search stack
            for (int i = 0; i < MAX_DEPTH; ++i) {
                (ss + i)->ply = i;
                (ss + i)->tt_hit = false;
                (ss + i)->static_eval = 0;
                (ss + i)->move_count = 0;
            }

            // if (depth >= 3) {
            //     alpha = std::max(-INF, score_avg - delta);
            //     beta = std::min(INF, score_avg + delta);
            // }

            auto start_current_search = time_ms();

            // Aspiration window search
            while (true) {
                if (to_play == WHITE)
                    score = search<WHITE, true>(m_root, ss, alpha, beta, depth);
                else
                    score = search<BLACK, true>(m_root, ss, alpha, beta, depth);
            
                if ((m_stop) ||
                    (m_cfg.timeset && time_ms() >= m_cfg.search_end_time) || 
                    (m_cfg.nodeset && m_info.nodes > m_cfg.nodeslimit)) 
                {
                    break;
                }
            
                if (score <= alpha) {
                    beta = (alpha + beta) / 2;
                    alpha = std::max(-INFTY, score - delta);
                    depth = root_depth;
                }
                else if (score >= beta) {
                    beta = std::min(INFTY, score + delta);
                    depth = std::max(depth - 1, root_depth - 5); 
                }
                else {
                    break;
                }

                delta *= 1.44;
            }

            score_avg = score_avg == 0 ? score : (score_avg + score) / 2;

            auto end_current_search = time_ms();

            // If we are out of time or over the limit for nodes (if there is one) then break
            if ((m_stop) ||
                (m_cfg.timeset && time_ms() >= m_cfg.search_end_time) || 
                (m_cfg.nodeset && m_info.nodes > m_cfg.nodeslimit)) {
                break;
            }
            
            uint32_t nps = (float)(m_info.nodes + m_info.qnodes) / ((end_current_search - start_current_search) / 1000.0f);   

            std::cout << "info depth " << current_depth << " score cp " << score * (to_play == WHITE ? 1 : -1) << " nodes " << m_info.nodes << " nps " << nps << " tthits " << m_tt.hits() << " qnodes " << m_info.qnodes << " BF " << std::pow(m_info.nodes, 1.0f / current_depth) << " pvlen " << m_ctx.pv_table_len[0] << " pv ";
            for (int j = 0; j < m_ctx.pv_table_len[0]; j++) {
                std::cout << m_ctx.pv_table[0][j] << " ";
            }
            std::cout << std::endl;

            best_move = m_ctx.pv_table[0][0];
            m_info = {0};
        }

        // Print the best move found
        std::cout << "bestmove ";
        std::cout << best_move;
        std::cout << std::endl;
    }

    // Explicit template definitions

    template int SEE<WHITE>(Position& pos, Square to);
    template int SEE<BLACK>(Position& pos, Square to);

    template int Worker::quiescence<WHITE, false>(Position& pos, int Aalpha, int Bbeta, int depth);
    template int Worker::quiescence<WHITE, true>(Position& pos, int Aalpha, int Bbeta, int depth);
    template int Worker::quiescence<BLACK, false>(Position& pos, int Aalpha, int Bbeta, int depth);
    template int Worker::quiescence<BLACK, true>(Position& pos, int Aalpha, int Bbeta, int depth);

    template int Worker::search<WHITE, false>(Position& pos, SearchStack *ss, int Aalpha, int Bbeta, int depth);
    template int Worker::search<WHITE, true>(Position& pos, SearchStack *ss, int Aalpha, int Bbeta, int depth);
    template int Worker::search<BLACK, false>(Position& pos, SearchStack *ss, int Aalpha, int Bbeta, int depth);
    template int Worker::search<BLACK, true>(Position& pos, SearchStack *ss, int Aalpha, int Bbeta, int depth);
} // namespace Search
