
#ifndef SEARCH_H 
#define SEARCH_H

#include <cmath>
#include <iostream>

#include <engine/movegen/Position.hpp>
#include <engine/movegen/Types.hpp>
#include <engine/search/Evaluate.hpp>
#include <engine/search/SearchTypes.hpp>
#include <engine/Log.hpp>
#include <engine/Misc.hpp>
#include <engine/movegen/Move.hpp>

#define INF 5000000

namespace Search {
    enum class WorkerState {
        IDLE = 0, SEARCHING, DEAD
    };

    class Worker {
        private:
            std::thread thread;

            std::mutex mutex;
            std::condition_variable cv;
            WorkerState state;

            Position root_pos;

            void idle_loop();
            void kill();

        public:
            Worker() : 
                state(WorkerState::IDLE),
                thread(&Worker::idle_loop, this)
            {};
            ~Worker() { kill(); };

            WorkerState get_state();
            void run(Position& pos, SearchConfig cfg);
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

    template <Color C, bool PVnode>
    int Quiescence(std::shared_ptr<SearchContext>& ctx, int Aalpha, int Bbeta, int depth);

    template <Color C, bool PVnode>
    int negamax(std::shared_ptr<SearchContext>& ctx, SearchStack *ss, int Aalpha, int Bbeta, int depth);

    template<Color C>
    int AspirationWindowSearch(SearchContext& ctx, SearchConfig cfg, const std::atomic<bool>& stop, SearchStack* ss, int score_avg, int depth);
    
    void Search(SearchContext& ctx, SearchConfig cfg, const std::atomic<bool>& stop);
} // namespace Search

#endif // SEARCH_H