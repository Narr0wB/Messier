
#ifndef THREAD_H
#define THREAD_H

#include <engine/movegen/Position.hpp>
#include <engine/search/SearchTypes.hpp>

#include <thread>
#include <atomic>
#include <functional>
#include <mutex>

enum class WorkerState {
    IDLE = 0, SEARCHING, DEAD
};

class SearchWorker {
    private:
        std::thread m_Thread;
        std::mutex  m_Mutex;
        std::condition_variable m_cv;
        std::atomic<bool> m_StopFlag;
		SearchContext m_SearchContext;
        WorkerState m_State;

        void idle_loop();
        void kill();

    public:
        SearchWorker() : 
            m_State(WorkerState::IDLE),
            m_Thread(&SearchWorker::idle_loop, this),
            m_SearchContext()
        {};
        ~SearchWorker() { kill(); };

        WorkerState get_state();
        void run(Position current, SearchInfo info);
        void stop();
        void clear();
};

#endif // THREAD_H