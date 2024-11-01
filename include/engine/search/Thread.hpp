
#ifndef THREAD_H
#define THREAD_H

#include <thread>

#include <engine/movegen/Position.hpp>
#include <engine/search/Search.hpp>

enum class ThreadState {
    IDLE = 0, SEARCHING
};

class SearchWorker {
    private:
        std::unique_ptr<std::thread> m_Thread;
        std::shared_ptr<SearchContext> m_SearchContext;
        ThreadState m_State;

    public:
        SearchWorker() : m_State(ThreadState::IDLE) {

        }

        ThreadState GetState();        
        void Stop();
        
        template <Color C>
        void Run(std::shared_ptr<SearchContext> ctx) {
            if (ctx == nullptr) {
              LOG_ERROR("Invalid search context, cannot start search worker!");
            }

            Stop();
            m_SearchContext = ctx;
            ctx->stop = false;
            
            m_State = ThreadState::SEARCHING;
            m_Thread = std::unique_ptr<std::thread>(new std::thread(&Search::Search<C>, ctx));
        }
};

#endif // THREAD_H

