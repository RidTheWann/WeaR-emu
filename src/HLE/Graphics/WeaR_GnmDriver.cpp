#include "WeaR_GnmDriver.h"
#include "GUI/WeaR_Logger.h"
#include "Graphics/WeaR_RenderEngine.h"
#include "Graphics/WeaR_RenderQueue.h"

#include <iostream>
#include <format>

namespace WeaR {

// =============================================================================
// GLOBAL INSTANCE
// =============================================================================

WeaR_GnmDriver& getGnmDriver() {
    static WeaR_GnmDriver instance;
    return instance;
}

// =============================================================================
// CONSTRUCTOR
// =============================================================================

WeaR_GnmDriver::WeaR_GnmDriver() {
    m_state.reset();
}

// =============================================================================
// LOGGING
// =============================================================================

void WeaR_GnmDriver::log(const std::string& message) {
    if (m_logger) {
        m_logger->log(message, LogLevel::Debug);
    } else {
        std::cout << "[GNM] " << message << "\n";
    }
}

// =============================================================================
// COMMAND BUFFER SUBMISSION
// =============================================================================

int32_t WeaR_GnmDriver::handleSubmitCommandBuffers(
    uint32_t count,
    uint64_t cmdBuffersPtr,
    uint64_t sizesPtr,
    WeaR_Memory& mem)
{
    log(std::format("SubmitCommandBuffers: count={}", count));

    for (uint32_t i = 0; i < count; ++i) {
        // Read buffer address and size from arrays
        uint64_t bufferAddr = mem.read<uint64_t>(cmdBuffersPtr + i * 8);
        uint32_t sizeInBytes = mem.read<uint32_t>(sizesPtr + i * 4);
        uint32_t sizeInDwords = sizeInBytes / 4;

        if (m_verbose) {
            log(std::format("  Buffer[{}]: addr=0x{:X}, size={} DWORDs", 
                            i, bufferAddr, sizeInDwords));
        }

        processCommandBuffer(bufferAddr, sizeInDwords, mem);
    }

    return 0;  // Success
}

// =============================================================================
// PM4 PARSER LOOP (CRITICAL)
// =============================================================================

void WeaR_GnmDriver::processCommandBuffer(
    uint64_t bufferAddr,
    uint32_t sizeInDwords,
    WeaR_Memory& mem)
{
    uint32_t offset = 0;

    while (offset < sizeInDwords) {
        // Read packet header
        uint32_t headerRaw = mem.read<uint32_t>(bufferAddr + offset * 4);
        PM4::PacketHeader header{headerRaw};
        offset++;

        if (!header.isType3()) {
            // Skip non-Type3 packets
            if (m_verbose) {
                log(std::format("PM4: Non-Type3 packet (type={}), skipping", 
                                static_cast<int>(header.type())));
            }
            continue;
        }

        uint8_t opcode = header.opcode();
        uint32_t payloadCount = header.payloadSize();

        // Safety check
        if (offset + payloadCount > sizeInDwords) {
            log(std::format("PM4: Packet overflow at offset {}", offset - 1));
            break;
        }

        // Read payload
        std::vector<uint32_t> payload(payloadCount);
        for (uint32_t i = 0; i < payloadCount; ++i) {
            payload[i] = mem.read<uint32_t>(bufferAddr + (offset + i) * 4);
        }

        // Log packet if verbose
        if (m_verbose) {
            log(std::format("PM4: {} (0x{:02X}), count={}", 
                            PM4::getOpcodeName(opcode), opcode, payloadCount));
        }

        // Dispatch to handler
        switch (opcode) {
            case PM4::Opcode::IT_NOP:
                handleNOP(payload.data(), payloadCount);
                break;

            case PM4::Opcode::IT_SET_CONTEXT_REG:
                handleSetContextReg(payload.data(), payloadCount, mem);
                break;

            case PM4::Opcode::IT_SET_SH_REG:
                handleSetShReg(payload.data(), payloadCount, mem);
                break;

            case PM4::Opcode::IT_DRAW_INDEX_AUTO:
                handleDrawIndexAuto(payload.data(), payloadCount);
                break;

            case PM4::Opcode::IT_DRAW_INDEX_2:
                handleDrawIndex2(payload.data(), payloadCount, mem);
                break;

            case PM4::Opcode::IT_DISPATCH_DIRECT:
                handleDispatchDirect(payload.data(), payloadCount);
                break;

            case PM4::Opcode::IT_EVENT_WRITE:
            case PM4::Opcode::IT_EVENT_WRITE_EOP:
                handleEventWrite(payload.data(), payloadCount);
                break;

            case PM4::Opcode::IT_ACQUIRE_MEM:
                handleAcquireMem(payload.data(), payloadCount);
                break;

            case PM4::Opcode::IT_RELEASE_MEM:
                handleReleaseMem(payload.data(), payloadCount);
                break;

            case PM4::Opcode::IT_INDEX_TYPE:
                handleIndexType(payload.data(), payloadCount);
                break;

            case PM4::Opcode::IT_NUM_INSTANCES:
                handleNumInstances(payload.data(), payloadCount);
                break;

            case PM4::Opcode::IT_INDIRECT_BUFFER:
                handleIndirectBuffer(payload.data(), payloadCount, mem);
                break;

            default:
                if (m_verbose) {
                    log(std::format("PM4: Unhandled opcode 0x{:02X} ({})", 
                                    opcode, PM4::getOpcodeName(opcode)));
                }
                break;
        }

        // Advance to next packet
        offset += payloadCount;
        m_packetsProcessed++;
    }
}

// =============================================================================
// DRAW COMMAND QUEUE
// =============================================================================

void WeaR_GnmDriver::queueDrawCommand(const DrawCommand& cmd) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_commandQueue.push(cmd);
    m_drawCallsQueued++;
}

std::vector<DrawCommand> WeaR_GnmDriver::flushDrawCommands() {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    std::vector<DrawCommand> commands;
    while (!m_commandQueue.empty()) {
        commands.push_back(m_commandQueue.front());
        m_commandQueue.pop();
    }
    return commands;
}

bool WeaR_GnmDriver::hasCommands() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return !m_commandQueue.empty();
}

// =============================================================================
// PM4 PACKET HANDLERS
// =============================================================================

void WeaR_GnmDriver::handleNOP([[maybe_unused]] const uint32_t* payload, 
                                [[maybe_unused]] uint32_t count) {
    // NOP - No operation, just for timing/alignment
}

void WeaR_GnmDriver::handleSetContextReg(const uint32_t* payload, uint32_t count,
                                          [[maybe_unused]] WeaR_Memory& mem) {
    if (count < 2) return;

    uint32_t regOffset = payload[0] & 0xFFFF;
    
    // Process each register value
    for (uint32_t i = 1; i < count; ++i) {
        uint32_t regIndex = regOffset + (i - 1);
        uint32_t value = payload[i];

        // Store in state based on register
        // (Simplified - real implementation would decode all registers)
        if (m_verbose) {
            log(std::format("  SET_CONTEXT_REG[0x{:04X}] = 0x{:08X}", regIndex, value));
        }
    }
}

void WeaR_GnmDriver::handleSetShReg(const uint32_t* payload, uint32_t count,
                                     [[maybe_unused]] WeaR_Memory& mem) {
    if (count < 2) return;

    uint32_t regOffset = payload[0] & 0xFFFF;

    for (uint32_t i = 1; i < count; ++i) {
        uint32_t regIndex = regOffset + (i - 1);
        uint32_t value = payload[i];

        if (m_verbose) {
            log(std::format("  SET_SH_REG[0x{:04X}] = 0x{:08X}", regIndex, value));
        }
    }
}

void WeaR_GnmDriver::handleDrawIndexAuto(const uint32_t* payload, uint32_t count) {
    if (count < 2) return;

    uint32_t vertexCount = payload[0];
    uint32_t drawInitiator = payload[1];
    (void)drawInitiator;

    // Create render queue command
    WeaR_DrawCmd cmd;
    cmd.type = RenderCmdType::DrawAuto;
    cmd.vertexCount = vertexCount;
    cmd.instanceCount = m_state.instanceCount;

    log(std::format("DRAW_INDEX_AUTO: vertices={}, instances={}", 
                    vertexCount, m_state.instanceCount));

    // Push to global render queue
    getRenderQueue().push(cmd);
    m_drawCallsQueued++;
}

void WeaR_GnmDriver::handleDrawIndex2(const uint32_t* payload, uint32_t count,
                                       [[maybe_unused]] WeaR_Memory& mem) {
    if (count < 4) return;

    uint32_t maxSize = payload[0];
    uint64_t indexBufferAddr = static_cast<uint64_t>(payload[1]) | 
                               (static_cast<uint64_t>(payload[2]) << 32);
    uint32_t indexCount = payload[3];
    (void)maxSize;

    m_state.indexBufferAddr = indexBufferAddr;

    // Create render queue command
    WeaR_DrawCmd cmd;
    cmd.type = RenderCmdType::DrawIndexed;
    cmd.indexCount = indexCount;
    cmd.indexBufferAddr = indexBufferAddr;
    cmd.instanceCount = m_state.instanceCount;
    cmd.indexType = m_state.indexType;

    log(std::format("DRAW_INDEX_2: indices={}, buffer=0x{:X}", 
                    indexCount, indexBufferAddr));

    getRenderQueue().push(cmd);
    m_drawCallsQueued++;
}

void WeaR_GnmDriver::handleDispatchDirect(const uint32_t* payload, uint32_t count) {
    if (count < 4) return;

    uint32_t threadGroupsX = payload[0];
    uint32_t threadGroupsY = payload[1];
    uint32_t threadGroupsZ = payload[2];

    // Create render queue command
    WeaR_DrawCmd cmd;
    cmd.type = RenderCmdType::ComputeDispatch;
    cmd.groupCountX = threadGroupsX;
    cmd.groupCountY = threadGroupsY;
    cmd.groupCountZ = threadGroupsZ;

    log(std::format("DISPATCH_DIRECT: groups={}x{}x{}", 
                    threadGroupsX, threadGroupsY, threadGroupsZ));

    getRenderQueue().push(cmd);
}

void WeaR_GnmDriver::handleEventWrite([[maybe_unused]] const uint32_t* payload,
                                       [[maybe_unused]] uint32_t count) {
    // GPU synchronization event - important for timing
    // For now, just acknowledge it
}

void WeaR_GnmDriver::handleAcquireMem([[maybe_unused]] const uint32_t* payload,
                                       [[maybe_unused]] uint32_t count) {
    // Memory barrier - ensure writes are visible
}

void WeaR_GnmDriver::handleReleaseMem([[maybe_unused]] const uint32_t* payload,
                                       [[maybe_unused]] uint32_t count) {
    // Memory barrier - signal completion
}

void WeaR_GnmDriver::handleIndexType(const uint32_t* payload, uint32_t count) {
    if (count < 1) return;
    m_state.indexType = payload[0] & 0x3;  // 0=16-bit, 1=32-bit
}

void WeaR_GnmDriver::handleNumInstances(const uint32_t* payload, uint32_t count) {
    if (count < 1) return;
    m_state.instanceCount = payload[0];
}

void WeaR_GnmDriver::handleIndirectBuffer(const uint32_t* payload, uint32_t count,
                                           WeaR_Memory& mem) {
    if (count < 3) return;

    uint64_t bufferAddr = static_cast<uint64_t>(payload[0]) | 
                          (static_cast<uint64_t>(payload[1] & 0xFFFF) << 32);
    uint32_t sizeInDwords = payload[2] & 0xFFFFF;

    log(std::format("INDIRECT_BUFFER: addr=0x{:X}, size={}", bufferAddr, sizeInDwords));

    // Recursively process the indirect buffer
    processCommandBuffer(bufferAddr, sizeInDwords, mem);
}

} // namespace WeaR
