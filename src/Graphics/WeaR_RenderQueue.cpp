#include "WeaR_RenderQueue.h"

#include <chrono>

namespace WeaR {

// =============================================================================
// GLOBAL INSTANCE
// =============================================================================

WeaR_RenderQueue& getRenderQueue() {
    static WeaR_RenderQueue instance;
    return instance;
}

// =============================================================================
// PRODUCER INTERFACE
// =============================================================================

void WeaR_RenderQueue::push(const WeaR_DrawCmd& cmd) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push_back(cmd);
        m_totalPushed++;
    }
    m_condVar.notify_one();
}

void WeaR_RenderQueue::push(const std::vector<WeaR_DrawCmd>& cmds) {
    if (cmds.empty()) return;
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& cmd : cmds) {
            m_queue.push_back(cmd);
        }
        m_totalPushed += cmds.size();
    }
    m_condVar.notify_one();
}

void WeaR_RenderQueue::endFrame() {
    WeaR_DrawCmd endCmd;
    endCmd.type = RenderCmdType::EndFrame;
    push(endCmd);
    m_frameCount++;
}

// =============================================================================
// CONSUMER INTERFACE
// =============================================================================

std::vector<WeaR_DrawCmd> WeaR_RenderQueue::popAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<WeaR_DrawCmd> result;
    result.reserve(m_queue.size());
    
    while (!m_queue.empty()) {
        result.push_back(std::move(m_queue.front()));
        m_queue.pop_front();
    }
    
    m_totalPopped += result.size();
    return result;
}

bool WeaR_RenderQueue::waitForCommands(uint32_t timeoutMs) {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    if (!m_queue.empty()) {
        return true;
    }
    
    auto timeout = std::chrono::milliseconds(timeoutMs);
    return m_condVar.wait_for(lock, timeout, [this] {
        return !m_queue.empty();
    });
}

bool WeaR_RenderQueue::isEmpty() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.empty();
}

size_t WeaR_RenderQueue::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size();
}

void WeaR_RenderQueue::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.clear();
}

} // namespace WeaR
