#pragma once

/**
 * @file WeaR_InternalBios.h
 * @brief Internal BIOS for testing without external game files
 * 
 * Writes a small test payload to memory that exercises basic syscalls.
 */

#include "WeaR_Memory.h"
#include "WeaR_Cpu.h"

#include <cstring>
#include <cstdint>

namespace WeaR {
namespace InternalBios {

/**
 * @brief Load internal BIOS into memory
 * @param mem Memory to write to
 * @param ctx CPU context to initialize
 * @return Entry point address
 */
inline uint64_t load(WeaR_Memory& mem, WeaR_Context& ctx) {
    // Entry point at 0x400000 (typical PS4 load address)
    constexpr uint64_t ENTRY_POINT = 0x400000;
    constexpr uint64_t STRING_ADDR = 0x400200;
    
    // Write boot message string
    const char* bootMsg = "WeaR-emu Internal BIOS v1.0\n";
    for (size_t i = 0; bootMsg[i]; ++i) {
        mem.write<uint8_t>(STRING_ADDR + i, static_cast<uint8_t>(bootMsg[i]));
    }
    mem.write<uint8_t>(STRING_ADDR + strlen(bootMsg), 0);  // Null terminator
    
    // Write test program at entry point
    // This program will:
    // 1. sys_write to stdout with boot message
    // 2. sceAudioOutInit
    // 3. Loop forever (idle)
    
    uint64_t addr = ENTRY_POINT;
    
    // =========================================================================
    // sys_write(1, bootMsg, strlen(bootMsg))
    // =========================================================================
    
    // MOV RAX, 4 (sys_write)
    mem.write<uint8_t>(addr++, 0x48);  // REX.W
    mem.write<uint8_t>(addr++, 0xC7);  // MOV r/m64, imm32
    mem.write<uint8_t>(addr++, 0xC0);  // RAX
    mem.write<uint32_t>(addr, 4);      // sys_write
    addr += 4;
    
    // MOV RDI, 1 (stdout)
    mem.write<uint8_t>(addr++, 0x48);  // REX.W
    mem.write<uint8_t>(addr++, 0xC7);  // MOV r/m64, imm32
    mem.write<uint8_t>(addr++, 0xC7);  // RDI
    mem.write<uint32_t>(addr, 1);      // fd = 1
    addr += 4;
    
    // MOV RSI, STRING_ADDR
    mem.write<uint8_t>(addr++, 0x48);  // REX.W
    mem.write<uint8_t>(addr++, 0xBE);  // MOV RSI, imm64
    mem.write<uint64_t>(addr, STRING_ADDR);
    addr += 8;
    
    // MOV RDX, strlen(bootMsg)
    mem.write<uint8_t>(addr++, 0x48);  // REX.W
    mem.write<uint8_t>(addr++, 0xC7);  // MOV r/m64, imm32
    mem.write<uint8_t>(addr++, 0xC2);  // RDX
    mem.write<uint32_t>(addr, static_cast<uint32_t>(strlen(bootMsg)));
    addr += 4;
    
    // SYSCALL
    mem.write<uint8_t>(addr++, 0x0F);
    mem.write<uint8_t>(addr++, 0x05);
    
    // =========================================================================
    // sceAudioOutInit()
    // =========================================================================
    
    // MOV RAX, 495 (sceAudioOutInit)
    mem.write<uint8_t>(addr++, 0x48);
    mem.write<uint8_t>(addr++, 0xC7);
    mem.write<uint8_t>(addr++, 0xC0);
    mem.write<uint32_t>(addr, 495);
    addr += 4;
    
    // SYSCALL
    mem.write<uint8_t>(addr++, 0x0F);
    mem.write<uint8_t>(addr++, 0x05);
    
    // =========================================================================
    // Idle loop - read input and do nothing
    // =========================================================================
    
    uint64_t loopStart = addr;
    
    // scePadReadState(0, 0x400300)
    // MOV RAX, 571
    mem.write<uint8_t>(addr++, 0x48);
    mem.write<uint8_t>(addr++, 0xC7);
    mem.write<uint8_t>(addr++, 0xC0);
    mem.write<uint32_t>(addr, 571);  // scePadReadState
    addr += 4;
    
    // MOV RDI, 0 (handle)
    mem.write<uint8_t>(addr++, 0x48);
    mem.write<uint8_t>(addr++, 0xC7);
    mem.write<uint8_t>(addr++, 0xC7);
    mem.write<uint32_t>(addr, 0);
    addr += 4;
    
    // MOV RSI, 0x400300 (output buffer)
    mem.write<uint8_t>(addr++, 0x48);
    mem.write<uint8_t>(addr++, 0xBE);
    mem.write<uint64_t>(addr, 0x400300);
    addr += 8;
    
    // SYSCALL
    mem.write<uint8_t>(addr++, 0x0F);
    mem.write<uint8_t>(addr++, 0x05);
    
    // PAUSE (yield CPU)
    mem.write<uint8_t>(addr++, 0xF3);
    mem.write<uint8_t>(addr++, 0x90);
    
    // JMP loopStart (relative jump back)
    int32_t offset = static_cast<int32_t>(loopStart - (addr + 5));
    mem.write<uint8_t>(addr++, 0xE9);  // JMP rel32
    mem.write<int32_t>(addr, offset);
    addr += 4;
    
    // =========================================================================
    // Initialize CPU context
    // =========================================================================
    
    ctx.RIP = ENTRY_POINT;
    ctx.RSP = PS4Memory::Region::STACK_TOP - 0x1000;
    ctx.RBP = ctx.RSP;
    ctx.RFLAGS = 0x202;  // IF set
    
    // Clear general purpose registers
    ctx.RAX = ctx.RBX = ctx.RCX = ctx.RDX = 0;
    ctx.RSI = ctx.RDI = 0;
    ctx.R8 = ctx.R9 = ctx.R10 = ctx.R11 = 0;
    ctx.R12 = ctx.R13 = ctx.R14 = ctx.R15 = 0;
    
    return ENTRY_POINT;
}

} // namespace InternalBios
} // namespace WeaR
