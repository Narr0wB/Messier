
#include <engine/search/Worker.hpp>
#include <engine/search/Search.hpp>


void SearchWorker::idle_loop() {
    while (true) {
        std::unique_lock<std::mutex> lock(m_Mutex);

        // Stop waiting on any state that is not idle
        m_cv.wait(lock, [&]{ return (m_State != WorkerState::IDLE); });
        lock.unlock();

        switch (m_State) {
            case WorkerState::DEAD: return; break;
            case WorkerState::SEARCHING: Search::Search(m_SearchContext, m_StopFlag); break;
        }

        lock.lock();
        m_State = WorkerState::IDLE;
        m_StopFlag = false;
    }
}

void SearchWorker::run(Position current, SearchInfo info) {
    // Stop any previous searches
    stop();
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_SearchContext.board = current;
    m_SearchContext.info  = info;
    m_State = WorkerState::SEARCHING;
    m_cv.notify_one();
}

void SearchWorker::stop() {
    std::unique_lock<std::mutex> lock(m_Mutex);
    if (m_State == WorkerState::SEARCHING) m_StopFlag = true;
    m_cv.notify_one();
}

void SearchWorker::clear() {
    stop();
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_SearchContext.clear();
}

void SearchWorker::kill() {
    stop();
    std::unique_lock<std::mutex> lock(m_Mutex);
    m_State = WorkerState::DEAD;
    m_cv.notify_one();
}

WorkerState SearchWorker::get_state() {
    return m_State;
}