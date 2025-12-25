#pragma once

/**
 * @file WeaR_Syscalls.h
 * @brief PS4/Orbis OS Syscall Dispatcher
 * 
 * Routes SYSCALL instructions to High-Level Emulation (HLE) handlers.
 * Implements System V AMD64 ABI calling convention.
 */

#include "Core/WeaR_Cpu.h"
#include "Core/WeaR_Memory.h"

#include <string>
#include <functional>
#include <unordered_map>
#include <cstdint>

namespace WeaR {

// Forward declaration
class WeaR_Logger;

// =============================================================================
// SYSCALL NUMBERS (FreeBSD/PS4)
// =============================================================================

namespace Syscall {
    // Standard FreeBSD syscalls
    constexpr uint64_t SYS_exit          = 1;
    constexpr uint64_t SYS_fork          = 2;
    constexpr uint64_t SYS_read          = 3;
    constexpr uint64_t SYS_write         = 4;
    constexpr uint64_t SYS_open          = 5;
    constexpr uint64_t SYS_close         = 6;
    constexpr uint64_t SYS_mmap          = 477;  // FreeBSD mmap
    constexpr uint64_t SYS_munmap        = 73;
    constexpr uint64_t SYS_mprotect      = 74;
    constexpr uint64_t SYS_ioctl         = 54;
    constexpr uint64_t SYS_getpid        = 20;
    constexpr uint64_t SYS_getuid        = 24;
    constexpr uint64_t SYS_gettimeofday  = 116;
    constexpr uint64_t SYS_nanosleep     = 240;
    
    // PS4-specific syscalls (600+)
    constexpr uint64_t SYS_sceKernelLoadStartModule     = 594;
    constexpr uint64_t SYS_sceKernelStopUnloadModule    = 595;
    constexpr uint64_t SYS_sceKernelDebugOut            = 602;
    constexpr uint64_t SYS_sceKernelGetModuleList       = 611;
    constexpr uint64_t SYS_sceKernelGetModuleInfo       = 612;
    constexpr uint64_t SYS_sceKernelIsNeoMode           = 618;
    constexpr uint64_t SYS_sceKernelGetCpuTemperature   = 621;
    
    // GNM Graphics syscalls
    constexpr uint64_t SYS_sceGnmSubmitCommandBuffers   = 591;
    constexpr uint64_t SYS_sceGnmSubmitDone             = 614;
    constexpr uint64_t SYS_sceGnmGetGpuCoreClockFrequency = 626;
}

// =============================================================================
// SYSCALL RESULT
// =============================================================================

struct SyscallResult {
    int64_t value = 0;     // Return value (goes to RAX)
    bool success = true;   // If false, value is negative errno
    std::string error;     // Error message for logging
};

// =============================================================================
// HLE FUNCTION SIGNATURE
// =============================================================================

// Arguments: RDI, RSI, RDX, R10, R8, R9
using HleFunction = std::function<SyscallResult(
    WeaR_Context& ctx,
    WeaR_Memory& mem,
    uint64_t rdi, uint64_t rsi, uint64_t rdx,
    uint64_t r10, uint64_t r8, uint64_t r9
)>;

// =============================================================================
// SYSCALL DISPATCHER CLASS
// =============================================================================

/**
 * @brief Central syscall dispatcher for HLE
 */
class WeaR_Syscalls {
public:
    WeaR_Syscalls();
    ~WeaR_Syscalls() = default;

    /**
     * @brief Dispatch a syscall based on context registers
     * @param ctx CPU context (read args, write result)
     * @param mem Memory for reading strings/buffers
     */
    void dispatch(WeaR_Context& ctx, WeaR_Memory& mem);

    /**
     * @brief Set logger for syscall output
     */
    void setLogger(WeaR_Logger* logger) { m_logger = logger; }

    /**
     * @brief Register a custom HLE handler
     */
    void registerHandler(uint64_t syscallNum, HleFunction handler);

    /**
     * @brief Get syscall name for debugging
     */
    [[nodiscard]] static std::string getSyscallName(uint64_t num);

    /**
     * @brief Get syscall statistics
     */
    [[nodiscard]] uint64_t getTotalCalls() const { return m_totalCalls; }
    [[nodiscard]] uint64_t getUnimplementedCalls() const { return m_unimplementedCalls; }

private:
    void registerDefaultHandlers();
    void log(const std::string& message);

    std::unordered_map<uint64_t, HleFunction> m_handlers;
    WeaR_Logger* m_logger = nullptr;
    uint64_t m_totalCalls = 0;
    uint64_t m_unimplementedCalls = 0;
};

// =============================================================================
// GLOBAL DISPATCHER ACCESS
// =============================================================================

/**
 * @brief Get global syscall dispatcher
 */
WeaR_Syscalls& getSyscallDispatcher();

} // namespace WeaR
