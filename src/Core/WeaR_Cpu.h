#pragma once

/**
 * @file WeaR_Cpu.h
 * @brief x86-64 CPU Core Emulation
 * 
 * Implements the PS4's AMD Jaguar x86-64 CPU:
 * - Full register file (GPR, XMM, special)
 * - Fetch-Decode-Execute cycle
 * - Thread-safe run loop
 */

#include <cstdint>
#include <array>
#include <atomic>
#include <functional>
#include <string>

namespace WeaR {

// Forward declaration
class WeaR_Memory;

// =============================================================================
// x86-64 FLAGS REGISTER
// =============================================================================

namespace Flags {
    constexpr uint64_t CF = 1ULL << 0;   // Carry
    constexpr uint64_t PF = 1ULL << 2;   // Parity
    constexpr uint64_t AF = 1ULL << 4;   // Auxiliary Carry
    constexpr uint64_t ZF = 1ULL << 6;   // Zero
    constexpr uint64_t SF = 1ULL << 7;   // Sign
    constexpr uint64_t TF = 1ULL << 8;   // Trap
    constexpr uint64_t IF = 1ULL << 9;   // Interrupt
    constexpr uint64_t DF = 1ULL << 10;  // Direction
    constexpr uint64_t OF = 1ULL << 11;  // Overflow
}

// =============================================================================
// CPU CONTEXT (Register File)
// =============================================================================

/**
 * @brief x86-64 CPU Register State
 */
struct WeaR_Context {
    // =========================================================================
    // General Purpose Registers (64-bit)
    // =========================================================================
    union {
        struct {
            uint64_t RAX;  // Accumulator
            uint64_t RBX;  // Base
            uint64_t RCX;  // Counter
            uint64_t RDX;  // Data
            uint64_t RSI;  // Source Index
            uint64_t RDI;  // Destination Index
            uint64_t RBP;  // Base Pointer
            uint64_t RSP;  // Stack Pointer
            uint64_t R8;
            uint64_t R9;
            uint64_t R10;
            uint64_t R11;
            uint64_t R12;
            uint64_t R13;
            uint64_t R14;
            uint64_t R15;
        };
        std::array<uint64_t, 16> GPR;  // Array access
    };

    // =========================================================================
    // Special Registers
    // =========================================================================
    uint64_t RIP;      // Instruction Pointer
    uint64_t RFLAGS;   // Flags Register

    // Segment Registers (simplified)
    uint16_t CS, DS, ES, FS, GS, SS;

    // =========================================================================
    // XMM Registers (128-bit SSE)
    // =========================================================================
    struct alignas(16) XMM {
        union {
            uint8_t  u8[16];
            uint16_t u16[8];
            uint32_t u32[4];
            uint64_t u64[2];
            float    f32[4];
            double   f64[2];
        };
    };
    std::array<XMM, 16> xmm;  // XMM0-XMM15

    // MXCSR (SSE control/status)
    uint32_t MXCSR;

    // =========================================================================
    // Helper Methods
    // =========================================================================
    
    void reset() {
        for (auto& r : GPR) r = 0;
        RIP = 0;
        RFLAGS = 0x202;  // IF set, reserved bit 1 set
        CS = DS = ES = FS = GS = SS = 0;
        for (auto& x : xmm) {
            for (auto& v : x.u64) v = 0;
        }
        MXCSR = 0x1F80;  // Default MXCSR
    }

    // Flag accessors
    [[nodiscard]] bool getFlag(uint64_t flag) const { return (RFLAGS & flag) != 0; }
    void setFlag(uint64_t flag, bool value) {
        if (value) RFLAGS |= flag;
        else RFLAGS &= ~flag;
    }

    [[nodiscard]] bool CF() const { return getFlag(Flags::CF); }
    [[nodiscard]] bool ZF() const { return getFlag(Flags::ZF); }
    [[nodiscard]] bool SF() const { return getFlag(Flags::SF); }
    [[nodiscard]] bool OF() const { return getFlag(Flags::OF); }
};

// =============================================================================
// CPU EXECUTION STATE
// =============================================================================

enum class CpuState : uint8_t {
    Stopped,
    Running,
    Paused,
    Halted,
    Faulted
};

// =============================================================================
// CPU CORE CLASS
// =============================================================================

/**
 * @brief x86-64 CPU Emulator Core
 */
class WeaR_Cpu {
public:
    using SyscallHandler = std::function<void(WeaR_Context&)>;

    explicit WeaR_Cpu(WeaR_Memory& memory);
    ~WeaR_Cpu();

    // Non-copyable
    WeaR_Cpu(const WeaR_Cpu&) = delete;
    WeaR_Cpu& operator=(const WeaR_Cpu&) = delete;

    // =========================================================================
    // Control
    // =========================================================================
    
    /**
     * @brief Start CPU execution loop (call from worker thread)
     */
    void runLoop();

    /**
     * @brief Execute single instruction
     * @return Number of cycles consumed (0 on halt/error)
     */
    uint32_t step();

    /**
     * @brief Stop execution loop
     */
    void stop();

    /**
     * @brief Pause execution
     */
    void pause();

    /**
     * @brief Resume from pause
     */
    void resume();

    /**
     * @brief Reset CPU to initial state
     */
    void reset();

    // =========================================================================
    // State Access
    // =========================================================================
    
    [[nodiscard]] CpuState getState() const { return m_state.load(); }
    [[nodiscard]] const WeaR_Context& getContext() const { return m_context; }
    [[nodiscard]] WeaR_Context& getContext() { return m_context; }
    [[nodiscard]] uint64_t getInstructionCount() const { return m_instructionCount.load(); }
    [[nodiscard]] uint8_t getLastOpcode() const { return m_lastOpcode; }

    /**
     * @brief Set syscall handler for INT/SYSCALL instructions
     */
    void setSyscallHandler(SyscallHandler handler) { m_syscallHandler = std::move(handler); }

    /**
     * @brief Get register name by index
     */
    [[nodiscard]] static std::string getRegisterName(size_t index);

private:
    // Fetch helpers
    uint8_t fetchByte();
    uint16_t fetchWord();
    uint32_t fetchDword();
    uint64_t fetchQword();

    // Decode helpers
    void decodeModRM(uint8_t modrm);
    void decodeSIB(uint8_t sib);

    // Instruction handlers (basic set for testing)
    void execNOP();
    void execRET();
    void execJMP_rel32();
    void execMOV_reg_imm64(uint8_t reg);
    void execPUSH_reg(uint8_t reg);
    void execPOP_reg(uint8_t reg);
    void execCALL_rel32();
    void execSYSCALL();
    void execHLT();
    void execUnknown(uint8_t opcode);

    WeaR_Memory& m_memory;
    WeaR_Context m_context{};
    
    std::atomic<CpuState> m_state{CpuState::Stopped};
    std::atomic<bool> m_shouldStop{false};
    std::atomic<uint64_t> m_instructionCount{0};
    
    uint8_t m_lastOpcode = 0;
    SyscallHandler m_syscallHandler;
};

} // namespace WeaR
