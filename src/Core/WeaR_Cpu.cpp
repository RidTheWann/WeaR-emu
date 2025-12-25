#include "WeaR_Cpu.h"
#include "WeaR_Memory.h"

#include <iostream>
#include <format>
#include <thread>
#include <chrono>

namespace WeaR {

// =============================================================================
// REGISTER NAMES
// =============================================================================

std::string WeaR_Cpu::getRegisterName(size_t index) {
    static const char* names[] = {
        "RAX", "RBX", "RCX", "RDX", "RSI", "RDI", "RBP", "RSP",
        "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15"
    };
    if (index < 16) return names[index];
    return std::format("R{}", index);
}

// =============================================================================
// CONSTRUCTOR / DESTRUCTOR
// =============================================================================

WeaR_Cpu::WeaR_Cpu(WeaR_Memory& memory)
    : m_memory(memory)
{
    reset();
}

WeaR_Cpu::~WeaR_Cpu() {
    stop();
}

// =============================================================================
// CONTROL
// =============================================================================

void WeaR_Cpu::reset() {
    m_context.reset();
    m_state.store(CpuState::Stopped);
    m_shouldStop.store(false);
    m_instructionCount.store(0);
    m_lastOpcode = 0;
    
    std::cout << "[CPU] Reset complete\n";
}

void WeaR_Cpu::stop() {
    m_shouldStop.store(true);
    m_state.store(CpuState::Stopped);
}

void WeaR_Cpu::pause() {
    if (m_state.load() == CpuState::Running) {
        m_state.store(CpuState::Paused);
    }
}

void WeaR_Cpu::resume() {
    if (m_state.load() == CpuState::Paused) {
        m_state.store(CpuState::Running);
    }
}

// =============================================================================
// RUN LOOP (Called from worker thread)
// =============================================================================

void WeaR_Cpu::runLoop() {
    std::cout << std::format("[CPU] Starting execution at RIP=0x{:016X}\n", m_context.RIP);
    
    m_state.store(CpuState::Running);
    m_shouldStop.store(false);

    while (!m_shouldStop.load()) {
        // Check for pause
        if (m_state.load() == CpuState::Paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Execute one instruction
        uint32_t cycles = step();
        
        if (cycles == 0) {
            // Halt or error occurred
            break;
        }

        m_instructionCount.fetch_add(1);

        // Basic cycle throttling (very simple for now)
        // In a real emulator, this would be more sophisticated
    }

    std::cout << std::format("[CPU] Execution stopped. Instructions: {}\n", 
                              m_instructionCount.load());
    
    if (m_state.load() != CpuState::Faulted) {
        m_state.store(CpuState::Stopped);
    }
}

// =============================================================================
// FETCH HELPERS
// =============================================================================

uint8_t WeaR_Cpu::fetchByte() {
    uint8_t value = m_memory.read<uint8_t>(m_context.RIP);
    m_context.RIP++;
    return value;
}

uint16_t WeaR_Cpu::fetchWord() {
    uint16_t value = m_memory.read<uint16_t>(m_context.RIP);
    m_context.RIP += 2;
    return value;
}

uint32_t WeaR_Cpu::fetchDword() {
    uint32_t value = m_memory.read<uint32_t>(m_context.RIP);
    m_context.RIP += 4;
    return value;
}

uint64_t WeaR_Cpu::fetchQword() {
    uint64_t value = m_memory.read<uint64_t>(m_context.RIP);
    m_context.RIP += 8;
    return value;
}

// =============================================================================
// SINGLE STEP (Fetch-Decode-Execute)
// =============================================================================

uint32_t WeaR_Cpu::step() {
    try {
        // =====================================================================
        // FETCH
        // =====================================================================
        uint8_t opcode = fetchByte();
        m_lastOpcode = opcode;

        // =====================================================================
        // DECODE & EXECUTE
        // =====================================================================
        
        // REX prefix detection (0x40-0x4F)
        bool hasREX = false;
        uint8_t rex = 0;
        if ((opcode & 0xF0) == 0x40) {
            hasREX = true;
            rex = opcode;
            opcode = fetchByte();
            m_lastOpcode = opcode;
        }

        switch (opcode) {
            // =================================================================
            // NOP (0x90)
            // =================================================================
            case 0x90:
                execNOP();
                return 1;

            // =================================================================
            // RET (0xC3)
            // =================================================================
            case 0xC3:
                execRET();
                return 1;

            // =================================================================
            // JMP rel32 (0xE9)
            // =================================================================
            case 0xE9:
                execJMP_rel32();
                return 1;

            // =================================================================
            // CALL rel32 (0xE8)
            // =================================================================
            case 0xE8:
                execCALL_rel32();
                return 1;

            // =================================================================
            // HLT (0xF4)
            // =================================================================
            case 0xF4:
                execHLT();
                return 0;  // Stop execution

            // =================================================================
            // PUSH reg (0x50-0x57)
            // =================================================================
            case 0x50: case 0x51: case 0x52: case 0x53:
            case 0x54: case 0x55: case 0x56: case 0x57:
                execPUSH_reg(opcode - 0x50);
                return 1;

            // =================================================================
            // POP reg (0x58-0x5F)
            // =================================================================
            case 0x58: case 0x59: case 0x5A: case 0x5B:
            case 0x5C: case 0x5D: case 0x5E: case 0x5F:
                execPOP_reg(opcode - 0x58);
                return 1;

            // =================================================================
            // MOV reg, imm64 (0xB8-0xBF with REX.W)
            // =================================================================
            case 0xB8: case 0xB9: case 0xBA: case 0xBB:
            case 0xBC: case 0xBD: case 0xBE: case 0xBF:
                if (hasREX && (rex & 0x08)) {  // REX.W
                    execMOV_reg_imm64(opcode - 0xB8 + (rex & 0x01 ? 8 : 0));
                } else {
                    // MOV reg, imm32 (zero-extended)
                    uint8_t reg = opcode - 0xB8;
                    uint32_t imm = fetchDword();
                    m_context.GPR[reg] = imm;
                }
                return 1;

            // =================================================================
            // Two-byte opcodes (0x0F prefix)
            // =================================================================
            case 0x0F: {
                uint8_t opcode2 = fetchByte();
                switch (opcode2) {
                    case 0x05:  // SYSCALL
                        execSYSCALL();
                        return 1;
                    default:
                        execUnknown(opcode2);
                        return 1;
                }
            }

            // =================================================================
            // Unknown opcode
            // =================================================================
            default:
                execUnknown(opcode);
                return 1;
        }
    }
    catch (const MemoryAccessException& e) {
        std::cerr << std::format("[CPU] Memory fault at RIP=0x{:016X}: {}\n",
                                  m_context.RIP, e.what());
        m_state.store(CpuState::Faulted);
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << std::format("[CPU] Exception: {}\n", e.what());
        m_state.store(CpuState::Faulted);
        return 0;
    }
}

// =============================================================================
// INSTRUCTION IMPLEMENTATIONS
// =============================================================================

void WeaR_Cpu::execNOP() {
    // NOP - No Operation
    // Just advances RIP (already done in fetch)
}

void WeaR_Cpu::execRET() {
    // RET - Return from procedure
    // Pop return address from stack
    uint64_t retAddr = m_memory.read<uint64_t>(m_context.RSP);
    m_context.RSP += 8;
    m_context.RIP = retAddr;
}

void WeaR_Cpu::execJMP_rel32() {
    // JMP rel32 - Jump relative
    int32_t offset = static_cast<int32_t>(fetchDword());
    m_context.RIP = static_cast<uint64_t>(
        static_cast<int64_t>(m_context.RIP) + offset
    );
}

void WeaR_Cpu::execCALL_rel32() {
    // CALL rel32 - Call procedure
    int32_t offset = static_cast<int32_t>(fetchDword());
    
    // Push return address
    m_context.RSP -= 8;
    m_memory.write<uint64_t>(m_context.RSP, m_context.RIP);
    
    // Jump to target
    m_context.RIP = static_cast<uint64_t>(
        static_cast<int64_t>(m_context.RIP) + offset
    );
}

void WeaR_Cpu::execMOV_reg_imm64(uint8_t reg) {
    // MOV reg, imm64 - Load 64-bit immediate
    uint64_t imm = fetchQword();
    if (reg < 16) {
        m_context.GPR[reg] = imm;
    }
}

void WeaR_Cpu::execPUSH_reg(uint8_t reg) {
    // PUSH reg - Push register to stack
    m_context.RSP -= 8;
    m_memory.write<uint64_t>(m_context.RSP, m_context.GPR[reg]);
}

void WeaR_Cpu::execPOP_reg(uint8_t reg) {
    // POP reg - Pop from stack to register
    m_context.GPR[reg] = m_memory.read<uint64_t>(m_context.RSP);
    m_context.RSP += 8;
}

void WeaR_Cpu::execSYSCALL() {
    // SYSCALL - System call
    if (m_syscallHandler) {
        m_syscallHandler(m_context);
    } else {
        std::cout << std::format("[CPU] SYSCALL RAX=0x{:X} (no handler)\n", 
                                  m_context.RAX);
    }
}

void WeaR_Cpu::execHLT() {
    // HLT - Halt processor
    std::cout << "[CPU] HLT instruction - stopping\n";
    m_state.store(CpuState::Halted);
}

void WeaR_Cpu::execUnknown(uint8_t opcode) {
    // Unknown opcode - log and continue
    std::cerr << std::format("[CPU] Unknown opcode 0x{:02X} at RIP=0x{:016X}\n",
                              opcode, m_context.RIP - 1);
}

// Placeholder implementations
void WeaR_Cpu::decodeModRM([[maybe_unused]] uint8_t modrm) {}
void WeaR_Cpu::decodeSIB([[maybe_unused]] uint8_t sib) {}

} // namespace WeaR
