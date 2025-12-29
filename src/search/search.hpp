
#ifndef SEARCH_H 
#define SEARCH_H

#include "../movegen/position.hpp"
#include "../movegen/types.hpp"
#include "../movegen/move.hpp"
#include "../search/tt.hpp"

#include <cmath>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>

#define MAX_DEPTH 20
#define MAX_TABLE MAX_DEPTH + 1

#define MATE_SCORE UINT16_MAX 

namespace Search {
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
            WorkerState m_state;

            SearchConfig m_cfg;
            SearchContext m_ctx;
            SearchInfo m_info;
            TTable& m_tt;

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
                m_ctx(),
                m_tt(table)
            {};
            ~Worker() { kill(); };

            WorkerState get_state();
            void run(Position& pos, SearchConfig cfg);
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
        const std::shared_ptr<SearchContext>& _ctx;
        int _ply;
        Move _ttmove;

        move_sorting_criterion(const std::shared_ptr<SearchContext>& c, int p, Move m) : _ctx(c), _ply(p), _ttmove(m) {}; 

        bool operator() (const Move& a, const Move& b) 
        {
            // Since the std::stable_sort function actually sorts elements in a list in ascending order by checking if a < b is true,  
            // we have to flip the comparison in order to have a list ordered in descending order 
            return score_move(a, _ctx, _ply, _ttmove) > score_move(b, _ctx, _ply, _ttmove);
        }
    };

    template <Color Us>
    void order_move_list(MoveList<Us>& m, const std::shared_ptr<SearchContext>& ctx, int ply, Move tt_move) 
    {
        // This function sorts the movelist in descending order
        std::stable_sort(m.begin(), m.end(), move_sorting_criterion<Us>(ctx, ply, tt_move));
    }

    template<Color C>
    int asp(SearchContext& ctx, const SearchConfig& cfg, SearchStack* ss, int score_avg, int depth);
    
} // namespace Search

#endif // SEARCH_H