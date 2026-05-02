
#include <atomic>
#include <algorithm>

#include "search/search.hpp" 
#include "search/evaluate.hpp"
#include "search/movepicker.hpp"
#include "misc.hpp"
#include "log.hpp"

#define DELTA_MARGIN 100

namespace Search {
    int LMReductions[MAX_DEPTH][64];

    const std::vector<std::string> BenchFENs = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", // Startpos
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", // Kiwipete
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", // Endgame
        "4rrk1/pp1n3p/3q2pQ/2p1pb2/2PP4/2P3N1/P2B2PP/4RRK1 b - - 5 20", // Middlegame tactics
        "rq3rk1/ppp2ppp/1b3B2/4p3/1P1n4/P1NP1bP1/2P1NP1P/R2Q1RK1 b - - 0 14",
        "r1bq1r1k/1pp1n1pp/1p1p4/4p2Q/4Pp2/1BNP4/PPP2PPP/3R1RK1 w - - 2 14"
    };

    void Worker::idle_loop() {
        while (true) {
            std::unique_lock<std::mutex> lock(m_mutex);

            // Stop waiting on any state that is not idle
            m_cv.wait(lock, [&]{ return (m_state != WorkerState::IDLE); });
            lock.unlock();

            m_stop = false;
            switch (m_state) {
                case WorkerState::SEARCHING: {
                    iterative_deepening(false); 
                    break;
                }
                case WorkerState::DEAD: {
                    return;
                }
            }

            lock.lock();
            m_state = WorkerState::IDLE;
            lock.unlock();
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
        lock.unlock();

        m_cv.notify_one();

        m_thread.join();
    }

    WorkerState Worker::get_state() {
        return m_state;
    }

    void Worker::bench(int depth) {
        uint64_t total_nodes = 0;
        uint64_t start_time = time_ms();

        m_tt.clear();

        std::cout << "Running benchmark... " << std::endl;

        for (const std::string& fen : BenchFENs) {
            Position::set(fen, m_root);

            m_info = { 0 };

            m_cfg.max_depth = depth;
            m_cfg.timeset = false;
            m_cfg.nodeset = false;

            iterative_deepening(false);

            total_nodes += m_info.nodes;
        }

        uint64_t end_time = time_ms();
        uint64_t elapsed = end_time - start_time;

        if (elapsed == 0) 
            elapsed = 1;

        uint64_t nps = (total_nodes * 1000) / elapsed;

        std::cout << "\n===========================\n";
        std::cout << "Total time (ms) : " << elapsed << "\n";
        std::cout << "Nodes searched  : " << total_nodes << "\n";
        std::cout << "Nodes/second    : " << nps << "\n";
    }

    void compute_lmr_reductions() {
        for (int depth = 0; depth < MAX_DEPTH; ++depth) {
            for (int move_count = 0; move_count < 64; ++move_count) {
                LMReductions[depth][move_count] = int(std::log(depth + 1) * std::log(move_count + 1));
            }
        }
    }
   


    template <Color C, bool PVnode>
    int Worker::quiescence(Position& pos, SearchStack* ss, int Aalpha, int Bbeta) 
    {
        m_info.qnodes++;
        m_info.nodes++;

        if ((m_stop) ||
            (m_cfg.timeset && (time_ms() >= m_cfg.search_end_time)) || 
            (m_cfg.nodeset && (m_info.nodes > m_cfg.nodeslimit))) 
        {
            return 0;
        }
        
        uint64_t hash = pos.get_hash();

        auto [tt_hit, tte] = m_tt.probe(hash);
        int tt_eval        = tte.eval;
        Move tt_move       = tte.move;
        int tt_score       = tte.score;
        uint8_t tt_bound   = tte.flags;
        int8_t tt_depth    = tte.depth;

        if (tt_hit && tt_score >= MATE_SCORE - MAX_PLY) 
            tt_score -= ss->ply; 
        else if (tt_hit && tt_score <= -MATE_SCORE + MAX_PLY)
            tt_score += ss->ply;

        // If we are not in a pv node, and we got a useful score from the TT, return early
        // NOTE: we do not check if the tte depth is greater (or equal) than the current depth because in quiescence any depth IS greater or equal than 0 
        if (!PVnode &&
            tt_hit && 
            ((tt_bound == FLAG_ALPHA && tt_score <= Aalpha) ||   
            (tt_bound == FLAG_BETA  && tt_score >= Bbeta) ||   
            (tt_bound == FLAG_EXACT))) 
        {
            m_info.tt_hits++;
            return tt_score;
        }

        int best_score = -INFTY;
        int score = 0;
        int move_count = 0;
        ss->in_check = pos.in_check<C>();
        Move m = Move::none();

        Transposition node(FLAG_ALPHA, hash, 0, NO_SCORE, NO_SCORE, Move::none(), m_info.generation);

        if (!ss->in_check) {
            // Stand pat, check if current position is already better than Beta (or atleast better than alfa)
            ss->static_eval = node.eval = (tt_hit && tt_eval != NO_SCORE) ? 
                tt_eval :
                corrected_eval<C>(pos); 

            best_score = ss->static_eval;
            node.score = best_score;

            if (best_score >= Bbeta) {
                node.flags = FLAG_BETA;
                m_tt.push(hash, node);
                return best_score;
            }
            if (best_score > Aalpha) {
                node.flags = FLAG_ALPHA;
                Aalpha = best_score;
            }

            // Futility pruning, if at frontier nodes we realize that the static evaluation of our position, even after adding the value of a knight, is still under alpha then 
            // prune this node by returning the static evaluation  
            if (!PVnode && ss->static_eval + piece_value[KNIGHT] < Aalpha) {
                node.flags = FLAG_ALPHA;
                node.score = ss->static_eval + piece_value[KNIGHT];
                m_tt.push(pos.get_hash(), node);
                return ss->static_eval + piece_value[KNIGHT];   
            }
        }
        else {
            ss->static_eval = node.eval = NO_SCORE;
        }
        
        if (ss->ply >= MAX_PLY - 1) 
            return best_score != -INFTY ? best_score : corrected_eval<C>(pos);


        MovePicker<C> picker(pos, m_ctx, ss->ply, -1, tt_move);
        while ((m = picker.next()) != Move::none()) {
            move_count++;

            if (!ss->in_check) {
                if (!m.is_capture() && !m.is_promotion()) 
                    continue;

                else if (!m.is_promotion() && ss->static_eval + piece_value[m.flags() == MoveFlags::EN_PASSANT ? PAWN : type_of(pos.at(m.to()))] + DELTA_MARGIN <= Aalpha)
                    continue;

                else if (m.is_promotion() && (m.promotion() != QUEEN))
                    continue;

                else if (!pos.see<C>(m, 0)) 
                    continue;
            }  
            else {
                if ((ss->ply - ss->qply) > 2 && m.is_quiet())
                    continue;
            }

            pos.play<C>(m);

            (ss + 1)->qply = ss->qply;
            score = -quiescence<~C, PVnode>(pos, ss + 1, -Bbeta, -Aalpha);

            pos.undo<C>(m);
            
            if ((m_stop) ||
                (m_cfg.timeset && (time_ms() >= m_cfg.search_end_time)) || 
                (m_cfg.nodeset && (m_info.nodes > m_cfg.nodeslimit))) 
            {
                return 0;
            }

            if (score > best_score) {
                best_score = score;
                node.score = score;
                node.move  = m;

                if (score >= MATE_SCORE - MAX_PLY)
                    node.score = score + ss->ply;
                else if (score <= -MATE_SCORE + MAX_PLY)
                    node.score = score - ss->ply;

                if (score > Aalpha) {
                    Aalpha = best_score;
                    node.flags = FLAG_EXACT;

                    if (score >= Bbeta) {
                        node.flags = FLAG_BETA;
                        break;
                    }
                }
            }
        }

        if (move_count == 0) {
            // If we are in check and there are no more moves available (i.e. best_score is still -INFTY), then we are in a checkmate
            best_score = ss->in_check ? -MATE_SCORE + ss->ply : 0;
            node.score = ss->in_check ? -MATE_SCORE : 0;
            node.flags = FLAG_EXACT;
        }

        m_tt.push(hash, node);
        return best_score;
    }



    /* Main search function */
    template <Color C, bool PVnode>
    int Worker::search(Position& pos, SearchStack *ss, int Aalpha, int Bbeta, int depth) 
    {
        m_info.nodes++;

        int ply = ss->ply;
        int move_count = 0;
        int score = 0;
        int best_score = -INFTY;
        ss->in_check = pos.in_check<C>();
        uint64_t hash = pos.get_hash();
        Move m = Move::none();

        // If depth is below zero, dip into quiescence search
        if (depth <= 0) {
            ss->qply = ply;
            return quiescence<C, PVnode>(pos, ss, Aalpha, Bbeta);
        }

        assert(-INFTY <= Aalpha && Aalpha < Bbeta && Bbeta <= INFTY);
        assert(0 < depth && depth < MAX_DEPTH + 1);
        assert(PVnode || (Aalpha == Bbeta - 1));

        // If out of time or hit any other constraints then exit the search
        if ((m_stop) ||
            (m_cfg.timeset && (time_ms() >= m_cfg.search_end_time)) || 
            (m_cfg.nodeset && (m_info.nodes > m_cfg.nodeslimit))) 
        {
            return 0;
        }


        if (ply != 0) {
            int limit = pos.ply() - pos.halfmove;
            if (limit < 0) limit = 0;
            int counter = 0;

            for (int i = pos.ply() - 2; i >= limit; i -= 2) {
                /*
                    If we find, in our linear board history, the current position, then we have proof that there exists a path (a sequence of moves)
                    that connects this position to itself. If we were to rexplore this position, we would explore the circular path, which would lead
                    us to this position again, triggering a three-fold repetition. We then assign the value 0 to this position (it's a draw)
                */


                if (hash == pos.history[i].hash && ++counter == 2) {
                    return 0;
                }
            }

            if (pos.halfmove == 50) return 0;

            // Mate distance pruning 
            // int alpha = std::max(mated_in(ply), Aalpha);
            // int beta  = std::min(mate_in(ply + 1), Bbeta);
            // if (alpha >= beta)
            //     return alpha;
        }

        auto [tt_hit, tte] = m_tt.probe(hash);
        int tt_eval      = tte.eval;
        Move tt_move     = tte.move;
        int tt_score     = tte.score;
        uint8_t tt_bound = tte.flags;
        int8_t tt_depth  = tte.depth;

        if (tt_hit && tt_score >= MATE_SCORE - MAX_PLY) 
            tt_score -= ss->ply; 
        else if (tt_hit && tt_score <= -MATE_SCORE + MAX_PLY)
            tt_score += ss->ply;

        if (tt_hit && 
            tt_depth >= depth &&
            ply != 0)
        {
            m_info.tt_hits++;
            if (tt_bound == FLAG_ALPHA && tt_score <= Aalpha) {
                return tt_score;
            }
            else if (tt_bound == FLAG_BETA && tt_score >= Bbeta) {
                return tt_score;
            }
            else if (tt_bound == FLAG_EXACT) {
                return tt_score;
            }
        }

        Transposition node(FLAG_ALPHA, hash, (int8_t)depth, NO_SCORE, ss->static_eval, Move::none(), m_info.generation);

        if (ss->in_check) {
            ss->static_eval = NO_SCORE;
        }
        else {
            ss->static_eval = (tt_hit && tt_eval != NO_SCORE) ? 
                tt_eval : 
                corrected_eval<C>(pos); 


            // Futility pruning, if at frontier nodes we realize that the static evaluation of our position, even after adding the value of a rook, is still under alpha then 
            // prune this node by returning the static evaluation  
            if (!PVnode 
                && depth == 1 
                && ss->static_eval + piece_value[ROOK] < Aalpha)
            {
                node.flags = FLAG_ALPHA;
                node.score = ss->static_eval + piece_value[ROOK];
                m_tt.push(pos.get_hash(), node);
                return ss->static_eval + piece_value[ROOK];   
            }

            int margin = (pos.npm() <= 6 ? 150 : 75) * depth;
            if (!PVnode 
                && depth <= 3 
                && ss->static_eval - margin >= Bbeta
                && pos.npm() > 6) // Margin scales with depth (e.g. 75cp per ply)
            {
                return ss->static_eval;
            }
        }


        // Null Move Pruning
        if (!PVnode 
            && !ss->in_check 
            && depth >= 4 
            && ss->static_eval >= Bbeta
            && pos.npm(C) >= 4) 
        {
            int NMPReduction = 3 + (depth / 6);

            pos.play_null_move();
            int v = -search<~C, false>(pos, ss + 1, -Bbeta, -Bbeta + 1, depth - 1 - NMPReduction);
            pos.undo_null_move();

            if (v >= Bbeta) {
                score = v >= MATE_SCORE - MAX_PLY ? Bbeta : v;
                // m_tt.push(hash, {FLAG_BETA, hash, (int8_t)depth, score, static_eval, Move::none(), pos.fen()});
                return score;
            }
        }

        MovePicker<C> picker(pos, m_ctx, ply, depth, tt_move);
        while ((m = picker.next()) != Move::none()) {
            // if (!pos.is_legal<C>(m))
            //     continue;

            move_count++;

            if (move_count > 2 && !ss->in_check && depth <= 3 && !pos.see<C>(m, -50 * depth)) 
                continue;

            pos.play<C>(m);

            // PVS
            if (move_count < 2) {
                score = -search<~C, PVnode>(pos, ss + 1, -Bbeta, -Aalpha, depth - 1);
            }
            else {
                // Late Move Reductions
                if (depth >= 3 
                    && move_count >= 3
                    && m.is_quiet() 
                    && (!ss->in_check && !pos.in_check<~C>()))
                {
                    int reduced = std::max(0, depth - 1 - LMReductions[std::min(depth, MAX_DEPTH - 1)][std::min(move_count, 63)]);
                    if (PVnode) reduced -= 1;
                    if (ss->in_check) reduced += 1;
                    reduced = std::clamp(reduced, 0, depth - 1);

                    score = -search<~C, false>(pos, ss + 1, -Aalpha - 1, -Aalpha, reduced);

                    if (score > Aalpha) 
                        score = -search<~C, false>(pos, ss + 1, -Aalpha - 1, -Aalpha, depth - 1);
                }
                else {
                    score = -search<~C, false>(pos, ss + 1, -Aalpha - 1, -Aalpha, depth - 1);
                }

                if (Aalpha < score && score < Bbeta)
                    score = -search<~C, true>(pos, ss + 1, -Bbeta, -Aalpha, depth - 1);
            }

            pos.undo<C>(m);

            if ((m_stop) ||
                (m_cfg.timeset && (time_ms() >= m_cfg.search_end_time)) || 
                (m_cfg.nodeset && (m_info.nodes > m_cfg.nodeslimit))) 
            {
                return 0;
            }

            if (score > best_score) {
                best_score = score;
                node.score = score;
                node.move  = m;

                if (score >= MATE_SCORE - MAX_PLY)
                    node.score = score + ss->ply;
                else if (score <= -MATE_SCORE + MAX_PLY)
                    node.score = score - ss->ply;

                // We have found a move that is better than the current alpha
                if (best_score > Aalpha) {
                    Aalpha = best_score;
                    node.flags = FLAG_EXACT;

                    // Fail High Node, i.e. we have found a move that is better than what our opponent is guaranteed to take
                    if (best_score >= Bbeta) {
                        if (m.is_quiet()) {
                            m_ctx.killer_moves[ply][1] = m_ctx.killer_moves[ply][0];
                            m_ctx.killer_moves[ply][0] = m;
                        }

                        // Rank history moves
                        if (m.is_quiet()) m_ctx.history_moves[static_cast<size_t>(C)][m.from()][m.to()] += depth;

                        node.flags = FLAG_BETA;
                        m_tt.push(hash, node);

                        return best_score;
                    }
                }
            }
        }

        if (move_count == 0) {
            // If we are in check and there are no more moves available (i.e. best_score is still -INFTY), then we are in a checkmate
            best_score = ss->in_check ? -MATE_SCORE + ss->ply : 0;
            node.score = ss->in_check ? -MATE_SCORE : 0;
            node.flags = FLAG_EXACT;
        }

        m_tt.push(hash, node);

        return best_score;
    }



    void Worker::iterative_deepening(bool silent = false) 
    {
        int max_depth = m_cfg.max_depth;
        int current_depth = 1;
        int last_score = NO_SCORE;
        uint64_t total_nodes = 0;
        uint64_t total_qnodes = 0;
        Color to_play = m_root.turn();
        Move best_move = Move::none();

        auto start_time = time_ms();

        for (; current_depth <= max_depth; ++current_depth) {
            int root_depth = m_cfg.max_depth = current_depth;
            int depth      = root_depth;
            int aw_margin  = 50;
            int alpha      = last_score != NO_SCORE ? last_score - aw_margin : -INFTY;
            int beta       = last_score != NO_SCORE ? last_score + aw_margin : INFTY;
            int score      = 0;
            m_info = {0};
            m_info.generation = m_root.ply();

            // Wipe search stack
            for (int i = 0; i < MAX_PLY; ++i) {
                (m_ss + i)->ply         = i;
                (m_ss + i)->qply        = 0;
                (m_ss + i)->static_eval = 0;
                (m_ss + i)->move_count  = 0;
                (m_ss + i)->tt_hit      = false;
                (m_ss + i)->in_check    = false;
            }

            auto start_current_search = time_ms();

            // Aspiration window search
            while (true) {
                m_info.aw_iterations++;

                if (to_play == WHITE) 
                    score = search<WHITE, true>(m_root, m_ss, alpha, beta, depth);
                else 
                    score = search<BLACK, true>(m_root, m_ss, alpha, beta, depth);

                if ((m_stop) ||
                    (m_cfg.timeset && time_ms() >= m_cfg.search_end_time) || 
                    (m_cfg.nodeset && m_info.nodes > m_cfg.nodeslimit)) 
                {
                    break;
                }

                aw_margin *= 2;

                if (score <= alpha) {
                    alpha = std::max<int64_t>(-INFTY, alpha - aw_margin);
                    if (m_info.aw_iterations == 3)
                        alpha = -INFTY;
                    // alpha = -INFTY;
                    // beta  = INFTY;
                }
                else if (score >= beta) {
                    beta = std::min<int64_t>(INFTY, beta + aw_margin);
                    if (m_info.aw_iterations == 3)
                        beta = INFTY;
                    // alpha = -INFTY;
                    // beta  = INFTY;
                }
                else {
                    break; 
                }
            }

            if ((m_stop) ||
                (m_cfg.timeset && time_ms() >= m_cfg.search_end_time) || 
                (m_cfg.nodeset && m_info.nodes > m_cfg.nodeslimit)) 
            {
                break;
            }

            last_score = score;

            auto end_current_search = time_ms();

            total_nodes += m_info.nodes;
            total_qnodes += m_info.qnodes;
            uint64_t elapsed = end_current_search - start_time;
            uint64_t nps = elapsed > 0 ? (total_nodes * 1000ULL) / elapsed : 0;   

            int pv_len = extract_pv();

            if (!silent) {
                std::cout 
                    << "info depth " << current_depth 
                    << " score cp " << score * (to_play == WHITE ? 1 : -1) 
                    << " nodes " << total_nodes 
                    << " qnodes " << total_qnodes 
                    << " nps " << nps 
                    << " tthits " << m_info.tt_hits 
                    << " ttstored " << m_tt.stored() 
                    << " BF " << std::pow(m_info.nodes, 1.0f / current_depth) 
                    << " AWit " << m_info.aw_iterations 
                    << " pvlen " << pv_len 
                    << " pv ";
                for (int j = 0; j < pv_len; j++) {
                    std::cout << m_pv[j] << " ";
                }
                std::cout << std::endl;
            }

            best_move = m_pv[0];
        }

        auto end_time = time_ms();

        if (!silent) {
            LOG_INFO("searchtime {}ms", (end_time - start_time));

            std::cout << "bestmove ";
            std::cout << best_move;
            std::cout << std::endl;
        }
    }
    
    int Worker::extract_pv() {
        int count = 0;
        Position pos = m_root;
        Color to_play = pos.turn();

        uint64_t visited[MAX_PLY];
        while (count < MAX_PLY) {
            // Cycle prevention
            // for (int i = 0; i < count; ++i) if (visited[i] == pos.get_hash()) return count;
            // visited[count] = pos.get_hash();

            auto [tt_hit, tte] = m_tt.probe(pos.get_hash());

            if (!tt_hit || tte.move == Move::none() || !pos.is_pseudo_legal(tte.move)) {
                // LOG_INFO("Broken PV: {}, {}, {}, {}", tt_hit, tte.move, pos.is_pseudo_legal(tte.move), pos.fen());
                break;
            }

            m_pv[count++] = tte.move;
            pos.play_dynamic(tte.move, to_play);
            to_play = ~to_play;
        }

        return count;
    }

    // Explicit template definitions

    template int Worker::quiescence<WHITE, false>(Position& pos, SearchStack *ss, int Aalpha, int Bbeta);
    template int Worker::quiescence<BLACK, false>(Position& pos, SearchStack *ss, int Aalpha, int Bbeta);
    template int Worker::quiescence<WHITE, true>(Position& pos, SearchStack *ss, int Aalpha, int Bbeta);
    template int Worker::quiescence<BLACK, true>(Position& pos, SearchStack *ss, int Aalpha, int Bbeta);

    template int Worker::search<WHITE, false>(Position& pos, SearchStack *ss, int Aalpha, int Bbeta, int depth);
    template int Worker::search<WHITE, true>(Position& pos, SearchStack *ss, int Aalpha, int Bbeta, int depth);
    template int Worker::search<BLACK, false>(Position& pos, SearchStack *ss, int Aalpha, int Bbeta, int depth);
    template int Worker::search<BLACK, true>(Position& pos, SearchStack *ss, int Aalpha, int Bbeta, int depth);
} // namespace Search
