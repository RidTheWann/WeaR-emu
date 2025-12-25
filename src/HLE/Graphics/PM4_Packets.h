#pragma once

/**
 * @file PM4_Packets.h
 * @brief AMD GCN/RDNA PM4 Packet Definitions
 * 
 * PM4 (Packet Manager 4) is the command protocol used by AMD GPUs.
 * PS4 uses AMD GCN architecture with PM4 Type-3 packets.
 */

#include <cstdint>

namespace WeaR {
namespace PM4 {

// =============================================================================
// PM4 PACKET TYPES
// =============================================================================

enum class PacketType : uint8_t {
    Type0 = 0,  // Register write (deprecated)
    Type1 = 1,  // Reserved
    Type2 = 2,  // Context switch
    Type3 = 3   // Command packet (most common)
};

// =============================================================================
// PM4 TYPE-3 HEADER FORMAT
// =============================================================================

// Header layout (32-bit):
// [31:30] = Type (3 for Type-3)
// [29:16] = Count (number of DWORDs - 1)
// [15:8]  = Opcode
// [7:0]   = Shader type / predicate

struct PacketHeader {
    uint32_t raw;

    [[nodiscard]] PacketType type() const {
        return static_cast<PacketType>((raw >> 30) & 0x3);
    }

    [[nodiscard]] uint16_t count() const {
        return static_cast<uint16_t>((raw >> 16) & 0x3FFF);
    }

    [[nodiscard]] uint8_t opcode() const {
        return static_cast<uint8_t>((raw >> 8) & 0xFF);
    }

    [[nodiscard]] uint8_t shaderType() const {
        return static_cast<uint8_t>(raw & 0xFF);
    }

    [[nodiscard]] bool isType3() const {
        return type() == PacketType::Type3;
    }

    [[nodiscard]] uint32_t payloadSize() const {
        return count() + 1;  // Count is N-1
    }
};

// =============================================================================
// PM4 TYPE-3 OPCODES (AMD GCN)
// =============================================================================

namespace Opcode {
    // Basic operations
    constexpr uint8_t IT_NOP                    = 0x10;
    constexpr uint8_t IT_WAIT_REG_MEM           = 0x3C;
    constexpr uint8_t IT_INDIRECT_BUFFER        = 0x3F;
    
    // Register writes
    constexpr uint8_t IT_SET_BASE               = 0x30;
    constexpr uint8_t IT_SET_SH_REG             = 0x76;
    constexpr uint8_t IT_SET_CONTEXT_REG        = 0x69;
    constexpr uint8_t IT_SET_UCONFIG_REG        = 0x79;
    
    // Draw commands
    constexpr uint8_t IT_INDEX_TYPE             = 0x2A;
    constexpr uint8_t IT_INDEX_BUFFER_SIZE      = 0x0A;
    constexpr uint8_t IT_DRAW_INDEX             = 0x2B;
    constexpr uint8_t IT_DRAW_INDEX_2           = 0x27;
    constexpr uint8_t IT_DRAW_INDEX_AUTO        = 0x2D;
    constexpr uint8_t IT_DRAW_INDEX_OFFSET_2    = 0x35;
    constexpr uint8_t IT_DRAW_INDEX_INDIRECT    = 0x38;
    
    // Dispatch commands
    constexpr uint8_t IT_DISPATCH_DIRECT        = 0x15;
    constexpr uint8_t IT_DISPATCH_INDIRECT      = 0x16;
    
    // Synchronization
    constexpr uint8_t IT_EVENT_WRITE            = 0x46;
    constexpr uint8_t IT_EVENT_WRITE_EOP        = 0x47;
    constexpr uint8_t IT_EVENT_WRITE_EOS        = 0x48;
    constexpr uint8_t IT_RELEASE_MEM            = 0x49;
    constexpr uint8_t IT_ACQUIRE_MEM            = 0x58;
    
    // Memory operations
    constexpr uint8_t IT_DMA_DATA               = 0x50;
    constexpr uint8_t IT_WRITE_DATA             = 0x37;
    constexpr uint8_t IT_MEM_SEMAPHORE          = 0x39;
    
    // State management
    constexpr uint8_t IT_CONTEXT_CONTROL        = 0x28;
    constexpr uint8_t IT_CLEAR_STATE            = 0x12;
    constexpr uint8_t IT_LOAD_SH_REG            = 0x77;
    constexpr uint8_t IT_LOAD_CONTEXT_REG       = 0x6A;
    
    // Primitive setup
    constexpr uint8_t IT_NUM_INSTANCES          = 0x2F;
    constexpr uint8_t IT_STRMOUT_BUFFER_UPDATE  = 0x34;
    
    // Copy/Blit
    constexpr uint8_t IT_COPY_DATA              = 0x40;
    constexpr uint8_t IT_SURFACE_SYNC           = 0x43;
}

// =============================================================================
// CONTEXT REGISTERS (Partial list)
// =============================================================================

namespace ContextReg {
    // Primitive Assembly
    constexpr uint32_t PA_SC_VPORT_SCISSOR_0_TL = 0x2800;
    constexpr uint32_t PA_SC_VPORT_SCISSOR_0_BR = 0x2801;
    constexpr uint32_t PA_SC_VPORT_ZMIN_0       = 0x2802;
    constexpr uint32_t PA_SC_VPORT_ZMAX_0       = 0x2803;
    
    // Color Buffer
    constexpr uint32_t CB_COLOR0_BASE           = 0xA318;
    constexpr uint32_t CB_COLOR0_VIEW           = 0xA31C;
    constexpr uint32_t CB_COLOR0_INFO           = 0xA31D;
    
    // Depth Buffer
    constexpr uint32_t DB_Z_INFO                = 0xA010;
    constexpr uint32_t DB_STENCIL_INFO          = 0xA011;
    constexpr uint32_t DB_HTILE_DATA_BASE       = 0xA014;
    
    // Vertex Fetch
    constexpr uint32_t VGT_VERTEX_REUSE_BLOCK_CNTL = 0xA2D5;
    constexpr uint32_t VGT_PRIMITIVE_TYPE       = 0xA2C7;
    
    // Shader
    constexpr uint32_t SPI_VS_OUT_CONFIG        = 0xA1B1;
    constexpr uint32_t SPI_PS_INPUT_CNTL_0      = 0xA191;
}

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

/**
 * @brief Get opcode name for debugging
 */
inline const char* getOpcodeName(uint8_t opcode) {
    switch (opcode) {
        case Opcode::IT_NOP:                return "IT_NOP";
        case Opcode::IT_SET_CONTEXT_REG:    return "IT_SET_CONTEXT_REG";
        case Opcode::IT_SET_SH_REG:         return "IT_SET_SH_REG";
        case Opcode::IT_SET_UCONFIG_REG:    return "IT_SET_UCONFIG_REG";
        case Opcode::IT_DRAW_INDEX:         return "IT_DRAW_INDEX";
        case Opcode::IT_DRAW_INDEX_2:       return "IT_DRAW_INDEX_2";
        case Opcode::IT_DRAW_INDEX_AUTO:    return "IT_DRAW_INDEX_AUTO";
        case Opcode::IT_DISPATCH_DIRECT:    return "IT_DISPATCH_DIRECT";
        case Opcode::IT_EVENT_WRITE:        return "IT_EVENT_WRITE";
        case Opcode::IT_EVENT_WRITE_EOP:    return "IT_EVENT_WRITE_EOP";
        case Opcode::IT_ACQUIRE_MEM:        return "IT_ACQUIRE_MEM";
        case Opcode::IT_RELEASE_MEM:        return "IT_RELEASE_MEM";
        case Opcode::IT_WAIT_REG_MEM:       return "IT_WAIT_REG_MEM";
        case Opcode::IT_WRITE_DATA:         return "IT_WRITE_DATA";
        case Opcode::IT_DMA_DATA:           return "IT_DMA_DATA";
        case Opcode::IT_INDIRECT_BUFFER:    return "IT_INDIRECT_BUFFER";
        case Opcode::IT_INDEX_TYPE:         return "IT_INDEX_TYPE";
        case Opcode::IT_NUM_INSTANCES:      return "IT_NUM_INSTANCES";
        case Opcode::IT_CONTEXT_CONTROL:    return "IT_CONTEXT_CONTROL";
        case Opcode::IT_CLEAR_STATE:        return "IT_CLEAR_STATE";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Build a Type-3 packet header
 */
inline uint32_t buildHeader(uint8_t opcode, uint16_t count, uint8_t shaderType = 0) {
    return (3u << 30) | ((count - 1) << 16) | (opcode << 8) | shaderType;
}

} // namespace PM4
} // namespace WeaR
