
#ifndef SEARCH_H 
#define SEARCH_H

#include "movegen/position.hpp"
#include "movegen/types.hpp"
#include "movegen/move.hpp"
#include "search/tt.hpp"

#include <cmath>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

#define MAX_DEPTH 25
#define MAX_PLY   30 
#define MAX_TABLE MAX_DEPTH + 1
#define MAX_HISTORY 8000

namespace Search {
    struct SearchConfig {
        uint64_t search_start_time;
        uint64_t search_end_time;
        uint64_t nodeslimit;
        uint32_t movestogo;
        Move searchmove;
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
        uint64_t aw_iterations;
        uint64_t tt_hits;
        uint64_t generation;
    };

    struct SearchStack {
        int  ply;
        int  qply;
        int  static_eval;
        int  move_count;
        bool in_check;
        bool tt_hit;
    };

    struct SearchContext {
        Move pv[MAX_TABLE];
        Move pv_table[MAX_TABLE][MAX_TABLE];
        int  pv_table_len[MAX_TABLE];
        Move killer_moves[MAX_TABLE][2];
        int  history_moves[2][64][64];
    };

    enum class WorkerState {
        IDLE = 0, SEARCHING, DEAD
    };

    class Worker {
        private:
            std::mutex m_mutex;
            std::condition_variable m_cv;
            std::atomic<bool> m_stop;
            std::atomic<bool> m_kill;

            WorkerState m_state;
            TTable& m_tt;

            Position m_root;

            uint32_t m_generation;
            SearchConfig m_cfg;
            SearchContext m_ctx;
            SearchInfo m_info;
            SearchStack m_ss[2 * MAX_PLY];

            std::array<Move, MAX_PLY> m_pv;

            std::thread m_thread;

            void idle_loop();
            void kill();

            void iterative_deepening(bool silent);
            bool exit_search();

            template <Color C, bool PVnode> int quiescence(Position& pos, SearchStack *ss, int Aalpha, int Bbeta);
            template <Color C, bool PVnode> int search(Position& pos, SearchStack *ss, int Aalpha, int Bbeta, int depth);
            template <Color C>              void update_history(const Move& m, int bonus);

        public:
            Worker(TTable& table) : 
                m_state(WorkerState::IDLE),
                m_stop(false),
                m_kill(false),
                m_generation(0),
                m_cfg(),
                m_ctx(),
                m_info(),
                m_tt(table),
                m_thread(&Worker::idle_loop, this)
            {}
            ~Worker() { kill(); }
		

            WorkerState get_state();
            void run(Position& pos, const SearchConfig& cfg);
            void bench(int depth);
            void clear();
            void stop();
    };

    /* Utility functions */
    int extract_pv(const Position& root, const TTable& table, std::array<Move, MAX_PLY>& out);
    void compute_lmx_parameters();

    inline int mate_in(int ply) { return MATE_SCORE - ply; };
    inline int mated_in(int ply) { return -MATE_SCORE + ply; };
} // namespace Search

#endif // SEARCH_H
