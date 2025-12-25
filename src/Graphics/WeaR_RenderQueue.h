#pragma once

/**
 * @file WeaR_RenderQueue.h
 * @brief Thread-Safe Render Command Queue
 * 
 * Bridges the HLE (CPU thread) with Vulkan (GPU thread).
 * Producer: WeaR_GnmDriver pushes commands
 * Consumer: WeaR_RenderEngine pops and executes
 */

#include <cstdint>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace WeaR {

// =============================================================================
// PIPELINE STATE
// =============================================================================

/**
 * @brief Current pipeline configuration
 */
struct WeaR_PipelineState {
    uint64_t vsShaderAddr = 0;     // Vertex shader
    uint64_t psShaderAddr = 0;     // Pixel/Fragment shader
    uint64_t csShaderAddr = 0;     // Compute shader
    
    uint32_t primitiveType = 4;    // 4 = Triangle list
    uint32_t cullMode = 0;         // 0 = None
    uint32_t frontFace = 0;        // 0 = CCW
    
    bool depthTestEnable = true;
    bool depthWriteEnable = true;
    bool blendEnable = false;
    
    // Compare for pipeline cache
    bool operator==(const WeaR_PipelineState& other) const {
        return vsShaderAddr == other.vsShaderAddr &&
               psShaderAddr == other.psShaderAddr &&
               primitiveType == other.primitiveType;
    }
};

// =============================================================================
// DRAW COMMAND TYPES
// =============================================================================

enum class RenderCmdType : uint8_t {
    None,
    Clear,
    SetPipeline,
    BindVertexBuffer,
    BindIndexBuffer,
    Draw,
    DrawIndexed,
    DrawAuto,
    ComputeDispatch,
    EndFrame
};

/**
 * @brief Single render command for the queue
 */
struct WeaR_DrawCmd {
    RenderCmdType type = RenderCmdType::None;
    
    // Draw parameters
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    uint32_t instanceCount = 1;
    uint32_t firstVertex = 0;
    uint32_t firstIndex = 0;
    uint32_t firstInstance = 0;
    int32_t  vertexOffset = 0;
    
    // Buffer bindings
    uint64_t vertexBufferAddr = 0;
    uint64_t indexBufferAddr = 0;
    uint32_t vertexStride = 0;
    uint32_t indexType = 0;  // 0=16-bit, 1=32-bit
    
    // Compute dispatch
    uint32_t groupCountX = 0;
    uint32_t groupCountY = 0;
    uint32_t groupCountZ = 0;
    
    // Clear parameters
    float clearColor[4] = {0, 0, 0, 1};
    float clearDepth = 1.0f;
    uint32_t clearStencil = 0;
    
    // Pipeline state (for SetPipeline)
    WeaR_PipelineState pipelineState;
};

// =============================================================================
// FRAME STATS
// =============================================================================

struct FrameStats {
    uint32_t drawCalls = 0;
    uint32_t dispatchCalls = 0;
    uint32_t triangleCount = 0;
    uint64_t vertexCount = 0;
};

// =============================================================================
// THREAD-SAFE RENDER QUEUE
// =============================================================================

/**
 * @brief Double-buffered thread-safe command queue
 */
class WeaR_RenderQueue {
public:
    WeaR_RenderQueue() = default;
    ~WeaR_RenderQueue() = default;

    // =========================================================================
    // Producer Interface (CPU/HLE Thread)
    // =========================================================================
    
    /**
     * @brief Push a single command (thread-safe)
     */
    void push(const WeaR_DrawCmd& cmd);
    
    /**
     * @brief Push multiple commands at once (thread-safe)
     */
    void push(const std::vector<WeaR_DrawCmd>& cmds);

    /**
     * @brief Signal end of frame from producer
     */
    void endFrame();

    // =========================================================================
    // Consumer Interface (Vulkan/Render Thread)
    // =========================================================================
    
    /**
     * @brief Pop all pending commands (thread-safe)
     * @return Vector of commands, empty if none pending
     */
    std::vector<WeaR_DrawCmd> popAll();

    /**
     * @brief Wait for commands with timeout
     * @param timeoutMs Maximum wait time in milliseconds
     * @return True if commands available
     */
    bool waitForCommands(uint32_t timeoutMs = 16);

    /**
     * @brief Check if empty without blocking
     */
    [[nodiscard]] bool isEmpty() const;

    /**
     * @brief Get pending command count
     */
    [[nodiscard]] size_t size() const;

    /**
     * @brief Clear all pending commands
     */
    void clear();

    // =========================================================================
    // Statistics
    // =========================================================================
    
    [[nodiscard]] uint64_t getTotalPushed() const { return m_totalPushed.load(); }
    [[nodiscard]] uint64_t getTotalPopped() const { return m_totalPopped.load(); }
    [[nodiscard]] uint64_t getFrameCount() const { return m_frameCount.load(); }

private:
    std::deque<WeaR_DrawCmd> m_queue;
    mutable std::mutex m_mutex;
    std::condition_variable m_condVar;
    
    std::atomic<uint64_t> m_totalPushed{0};
    std::atomic<uint64_t> m_totalPopped{0};
    std::atomic<uint64_t> m_frameCount{0};
};

/**
 * @brief Get global render queue instance
 */
WeaR_RenderQueue& getRenderQueue();

} // namespace WeaR
