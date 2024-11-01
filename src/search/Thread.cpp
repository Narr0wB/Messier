
#include <engine/search/Thread.hpp>

ThreadState SearchWorker::GetState() {
    return m_State;
}

void SearchWorker::Stop() {
    if (m_State == ThreadState::IDLE || m_Thread == nullptr) return;
    
    if (m_SearchContext) { m_SearchContext->stop = true; }

    if (m_Thread->joinable())
        m_Thread->join();
        
    m_State = ThreadState::IDLE;
}