#pragma once

/**
 * @file WeaR_GnmDriver.h
 * @brief PS4 GNM Graphics Driver Emulation
 * 
 * Translates PS4 GPU Command Buffers (PM4 packets) to Vulkan calls.
 * Acts as the bridge between HLE syscalls and WeaR_RenderEngine.
 */

#include "PM4_Packets.h"
#include "Core/WeaR_Memory.h"

#include <cstdint>
#include <vector>
#include <queue>
#include <mutex>

namespace WeaR {

// Forward declarations
class WeaR_RenderEngine;
class WeaR_Logger;

// =============================================================================
// DRAW COMMAND TYPES
// =============================================================================

enum class DrawCommandType : uint8_t {
    Clear,
    DrawIndexed,
    DrawAuto,
    Dispatch
};

/**
 * @brief Queued draw command for render engine
 */
struct DrawCommand {
    DrawCommandType type;
    uint32_t vertexCount = 0;
    uint32_t instanceCount = 1;
    uint32_t indexCount = 0;
    uint64_t indexBuffer = 0;
    uint64_t vertexBuffer = 0;
    uint32_t primitiveType = 0;  // Triangle, etc.
};

// =============================================================================
// GPU STATE (Tracked from PM4 packets)
// =============================================================================

struct GpuState {
    // Vertex buffers
    uint64_t vertexBufferAddr[16] = {};
    uint32_t vertexBufferStride[16] = {};
    
    // Index buffer
    uint64_t indexBufferAddr = 0;
    uint32_t indexBufferSize = 0;
    uint32_t indexType = 0;  // 16-bit or 32-bit
    
    // Render targets
    uint64_t colorTargetAddr[8] = {};
    uint32_t colorTargetFormat[8] = {};
    uint64_t depthTargetAddr = 0;
    
    // Shaders (addresses)
    uint64_t vsShaderAddr = 0;
    uint64_t psShaderAddr = 0;
    uint64_t csShaderAddr = 0;
    
    // Draw state
    uint32_t primitiveType = 4;  // Default: Triangle List
    uint32_t instanceCount = 1;
    
    // Viewport
    float viewportX = 0, viewportY = 0;
    float viewportWidth = 1920, viewportHeight = 1080;
    float viewportMinZ = 0, viewportMaxZ = 1;
    
    void reset() {
        *this = GpuState{};
    }
};

// =============================================================================
// GNM DRIVER CLASS
// =============================================================================

/**
 * @brief PS4 GNM GPU Command Processor
 */
class WeaR_GnmDriver {
public:
    WeaR_GnmDriver();
    ~WeaR_GnmDriver() = default;

    /**
     * @brief Set dependencies
     */
    void setRenderEngine(WeaR_RenderEngine* engine) { m_renderEngine = engine; }
    void setLogger(WeaR_Logger* logger) { m_logger = logger; }

    /**
     * @brief Handle sceGnmSubmitCommandBuffers syscall
     * @param count Number of command buffers
     * @param cmdBuffersPtr Array of command buffer pointers
     * @param sizesPtr Array of command buffer sizes
     * @param mem Memory for reading buffers
     * @return 0 on success
     */
    int32_t handleSubmitCommandBuffers(
        uint32_t count,
        uint64_t cmdBuffersPtr,
        uint64_t sizesPtr,
        WeaR_Memory& mem
    );

    /**
     * @brief Process a single command buffer
     */
    void processCommandBuffer(
        uint64_t bufferAddr,
        uint32_t sizeInDwords,
        WeaR_Memory& mem
    );

    /**
     * @brief Get queued draw commands (thread-safe)
     */
    std::vector<DrawCommand> flushDrawCommands();

    /**
     * @brief Check if commands are pending
     */
    [[nodiscard]] bool hasCommands() const;

    /**
     * @brief Get current GPU state snapshot
     */
    [[nodiscard]] const GpuState& getState() const { return m_state; }

    /**
     * @brief Statistics
     */
    [[nodiscard]] uint64_t getPacketsProcessed() const { return m_packetsProcessed; }
    [[nodiscard]] uint64_t getDrawCallsQueued() const { return m_drawCallsQueued; }

private:
    void log(const std::string& message);
    
    // PM4 packet handlers
    void handleNOP(const uint32_t* payload, uint32_t count);
    void handleSetContextReg(const uint32_t* payload, uint32_t count, WeaR_Memory& mem);
    void handleSetShReg(const uint32_t* payload, uint32_t count, WeaR_Memory& mem);
    void handleDrawIndexAuto(const uint32_t* payload, uint32_t count);
    void handleDrawIndex2(const uint32_t* payload, uint32_t count, WeaR_Memory& mem);
    void handleDispatchDirect(const uint32_t* payload, uint32_t count);
    void handleEventWrite(const uint32_t* payload, uint32_t count);
    void handleAcquireMem(const uint32_t* payload, uint32_t count);
    void handleReleaseMem(const uint32_t* payload, uint32_t count);
    void handleIndexType(const uint32_t* payload, uint32_t count);
    void handleNumInstances(const uint32_t* payload, uint32_t count);
    void handleIndirectBuffer(const uint32_t* payload, uint32_t count, WeaR_Memory& mem);

    void queueDrawCommand(const DrawCommand& cmd);

    WeaR_RenderEngine* m_renderEngine = nullptr;
    WeaR_Logger* m_logger = nullptr;
    
    GpuState m_state;
    
    std::queue<DrawCommand> m_commandQueue;
    mutable std::mutex m_queueMutex;
    
    uint64_t m_packetsProcessed = 0;
    uint64_t m_drawCallsQueued = 0;
    bool m_verbose = true;  // Log all packets for debugging
};

/**
 * @brief Get global GNM driver instance
 */
WeaR_GnmDriver& getGnmDriver();

} // namespace WeaR
