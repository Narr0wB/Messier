
#ifndef SEARCH_H 
#define SEARCH_H

#include "../movegen/position.hpp"
#include "../movegen/types.hpp"
#include "../movegen/move.hpp"
#include "src/search/tt.hpp"

#include <cmath>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>

#define MAX_DEPTH 20
#define MAX_TABLE MAX_DEPTH + 1

#define MATE_SCORE UINT16_MAX 
#define INFTY (MATE_SCORE + 1) 

namespace Search {
    struct SearchConfig {
        uint64_t search_start_time;
        uint64_t search_end_time;
        uint64_t nodeslimit;
        uint32_t movestogo;
        int max_depth;
        int quiescence_depth;
        bool timeset;
        bool movetimeset;
        bool nodeset;
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
        Move pv_table[MAX_TABLE][MAX_TABLE];
        int pv_table_len[MAX_TABLE];
        Move killer_moves[MAX_TABLE][2];
        int history_moves[64][64];
    };

    enum class WorkerState {
        IDLE = 0, SEARCHING, DEAD
    };

    class Worker {
        private:
            std::thread m_thread;
            std::mutex m_mutex;
            std::condition_variable m_cv;
            std::atomic<bool> m_stop;
            WorkerState m_state;
            TTable& m_tt;

            // Position is reset after call to Worker::run()
            Position m_root;

            // Structure is reset after call to Worker::run()
            SearchConfig m_cfg;

            // Structure is reset after call to Worker::clear()
            SearchContext m_ctx;

            // Structure is reset after each iteration in Worker::iterative_deepening()
            SearchInfo m_info;

            void idle_loop();
            void kill();

            void iterative_deepening();

            template <Color C, bool PVnode>
            int quiescence(Position& pos, int Aalpha, int Bbeta, int depth);
            template <Color C, bool PVnode>
            int search(Position& ctx, SearchStack *ss, int Aalpha, int Bbeta, int depth);

        public:
            Worker(TTable& table) : 
                m_state(WorkerState::IDLE),
                m_thread(&Worker::idle_loop, this),
                m_cfg(),
                m_ctx(),
                m_info(),
                m_tt(table)
            {}
            ~Worker() { kill(); }

            WorkerState get_state();
            void run(Position& pos, const SearchConfig& cfg);
            void clear();
            void stop();
    };

    template <Color Us>
    int SEE(Position& pos, Square to);

    int mate_in(int ply);
    int mated_in(int ply);

    // Order moves using context
    template <Color Us>
    struct move_sorting_criterion {
        const SearchContext& ctx;
        const Position& pos;
        int ply;
        Move tt_move;

        move_sorting_criterion(const SearchContext& c, const Position& p, int pl, Move m) : 
            ctx(c), 
            pos(p),
            ply(pl), 
            tt_move(m)
        {} 

        bool operator() (const Move& a, const Move& b) 
        {
            // Since the std::stable_sort function actually sorts elements in a list in ascending order by checking if a < b is true,  
            // we have to flip the comparison in order to have a list ordered in descending order 
            return score_move(a, ctx, pos, ply, tt_move) > score_move(b, ctx, pos, ply, tt_move);
        }
    };

    template <Color Us>
    void order_move_list(MoveList<Us>& m, const SearchContext& ctx, const Position& pos, int ply, Move tt_move) 
    {
        // This function sorts the movelist in descending order
        std::stable_sort(m.begin(), m.end(), move_sorting_criterion<Us>(ctx, pos, ply, tt_move));
    }
} // namespace Search

#endif // SEARCH_H