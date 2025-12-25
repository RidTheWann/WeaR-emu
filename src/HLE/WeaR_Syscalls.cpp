#include "WeaR_Syscalls.h"
#include "GUI/WeaR_Logger.h"
#include "Graphics/WeaR_GnmDriver.h"

#include <iostream>
#include <format>
#include <cstring>

namespace WeaR {

// =============================================================================
// GLOBAL DISPATCHER
// =============================================================================

WeaR_Syscalls& getSyscallDispatcher() {
    static WeaR_Syscalls instance;
    return instance;
}

// =============================================================================
// SYSCALL NAMES
// =============================================================================

std::string WeaR_Syscalls::getSyscallName(uint64_t num) {
    switch (num) {
        case Syscall::SYS_exit:          return "sys_exit";
        case Syscall::SYS_fork:          return "sys_fork";
        case Syscall::SYS_read:          return "sys_read";
        case Syscall::SYS_write:         return "sys_write";
        case Syscall::SYS_open:          return "sys_open";
        case Syscall::SYS_close:         return "sys_close";
        case Syscall::SYS_mmap:          return "sys_mmap";
        case Syscall::SYS_munmap:        return "sys_munmap";
        case Syscall::SYS_mprotect:      return "sys_mprotect";
        case Syscall::SYS_ioctl:         return "sys_ioctl";
        case Syscall::SYS_getpid:        return "sys_getpid";
        case Syscall::SYS_getuid:        return "sys_getuid";
        case Syscall::SYS_gettimeofday:  return "sys_gettimeofday";
        case Syscall::SYS_nanosleep:     return "sys_nanosleep";
        case Syscall::SYS_sceKernelLoadStartModule:  return "sceKernelLoadStartModule";
        case Syscall::SYS_sceKernelStopUnloadModule: return "sceKernelStopUnloadModule";
        case Syscall::SYS_sceKernelDebugOut:         return "sceKernelDebugOut";
        case Syscall::SYS_sceKernelGetModuleList:    return "sceKernelGetModuleList";
        case Syscall::SYS_sceKernelGetModuleInfo:    return "sceKernelGetModuleInfo";
        case Syscall::SYS_sceKernelIsNeoMode:        return "sceKernelIsNeoMode";
        case Syscall::SYS_sceKernelGetCpuTemperature:return "sceKernelGetCpuTemperature";
        default: return std::format("syscall_{}", num);
    }
}

// =============================================================================
// CONSTRUCTOR
// =============================================================================

WeaR_Syscalls::WeaR_Syscalls() {
    registerDefaultHandlers();
}

// =============================================================================
// LOGGING
// =============================================================================

void WeaR_Syscalls::log(const std::string& message) {
    if (m_logger) {
        m_logger->log(message);
    } else {
        std::cout << "[Syscall] " << message << "\n";
    }
}

// =============================================================================
// DISPATCH
// =============================================================================

void WeaR_Syscalls::dispatch(WeaR_Context& ctx, WeaR_Memory& mem) {
    m_totalCalls++;

    uint64_t syscallNum = ctx.RAX;
    uint64_t rdi = ctx.RDI;
    uint64_t rsi = ctx.RSI;
    uint64_t rdx = ctx.RDX;
    uint64_t r10 = ctx.R10;
    uint64_t r8  = ctx.R8;
    uint64_t r9  = ctx.R9;

    auto it = m_handlers.find(syscallNum);
    if (it != m_handlers.end()) {
        SyscallResult result = it->second(ctx, mem, rdi, rsi, rdx, r10, r8, r9);
        ctx.RAX = static_cast<uint64_t>(result.value);
        
        if (!result.success) {
            log(std::format("{}: {}", getSyscallName(syscallNum), result.error));
        }
    } else {
        // Unimplemented syscall
        m_unimplementedCalls++;
        log(std::format("Unimplemented syscall: {} ({})", 
                        getSyscallName(syscallNum), syscallNum));
        
        // Return 0 (success) to allow game to continue
        ctx.RAX = 0;
    }
}

void WeaR_Syscalls::registerHandler(uint64_t syscallNum, HleFunction handler) {
    m_handlers[syscallNum] = std::move(handler);
}

// =============================================================================
// DEFAULT HANDLERS
// =============================================================================

void WeaR_Syscalls::registerDefaultHandlers() {
    // =========================================================================
    // sys_exit (1)
    // =========================================================================
    registerHandler(Syscall::SYS_exit, [this](
        WeaR_Context& ctx, WeaR_Memory&,
        uint64_t rdi, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)
    {
        log(std::format("sys_exit(status={})", static_cast<int>(rdi)));
        // Signal CPU to stop (handled externally)
        ctx.RAX = 0;
        return SyscallResult{0, true, ""};
    });

    // =========================================================================
    // sys_write (4)
    // =========================================================================
    registerHandler(Syscall::SYS_write, [this](
        WeaR_Context&, WeaR_Memory& mem,
        uint64_t fd, uint64_t buf, uint64_t count, uint64_t, uint64_t, uint64_t)
    {
        // Read string from memory
        std::string output;
        output.reserve(count);
        
        try {
            for (uint64_t i = 0; i < count && i < 4096; ++i) {  // Limit to 4KB
                char c = static_cast<char>(mem.read<uint8_t>(buf + i));
                if (c == '\0') break;
                output += c;
            }
        } catch (...) {
            return SyscallResult{-14, false, "EFAULT: Bad address"};  // EFAULT
        }

        // Log to kernel console
        if (fd == 1 || fd == 2) {  // stdout or stderr
            log(std::format("[fd{}] {}", fd, output));
        }

        return SyscallResult{static_cast<int64_t>(output.size()), true, ""};
    });

    // =========================================================================
    // sys_mmap (477)
    // =========================================================================
    registerHandler(Syscall::SYS_mmap, [this](
        WeaR_Context&, WeaR_Memory&,
        uint64_t addr, uint64_t length, uint64_t prot, 
        uint64_t flags, uint64_t fd, uint64_t offset)
    {
        (void)prot; (void)flags; (void)fd; (void)offset;
        
        // Simplified: return a fixed address in heap region
        static uint64_t nextAlloc = PS4Memory::Region::HEAP_BASE;
        
        uint64_t allocAddr = (addr != 0) ? addr : nextAlloc;
        uint64_t alignedLen = (length + 0xFFF) & ~0xFFFULL;  // Page align
        
        nextAlloc += alignedLen;

        log(std::format("sys_mmap(addr=0x{:X}, len={}) -> 0x{:X}", 
                        addr, length, allocAddr));

        return SyscallResult{static_cast<int64_t>(allocAddr), true, ""};
    });

    // =========================================================================
    // sys_getpid (20)
    // =========================================================================
    registerHandler(Syscall::SYS_getpid, [](
        WeaR_Context&, WeaR_Memory&,
        uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)
    {
        return SyscallResult{1000, true, ""};  // Fake PID
    });

    // =========================================================================
    // sys_getuid (24)
    // =========================================================================
    registerHandler(Syscall::SYS_getuid, [](
        WeaR_Context&, WeaR_Memory&,
        uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)
    {
        return SyscallResult{0, true, ""};  // Root UID
    });

    // =========================================================================
    // sceKernelDebugOut (602)
    // =========================================================================
    registerHandler(Syscall::SYS_sceKernelDebugOut, [this](
        WeaR_Context&, WeaR_Memory& mem,
        uint64_t msgPtr, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)
    {
        // Read null-terminated string from memory
        std::string message;
        try {
            for (uint64_t i = 0; i < 1024; ++i) {
                char c = static_cast<char>(mem.read<uint8_t>(msgPtr + i));
                if (c == '\0') break;
                message += c;
            }
        } catch (...) {
            return SyscallResult{-14, false, "EFAULT"};
        }

        log(std::format("[DEBUG] {}", message));
        return SyscallResult{0, true, ""};
    });

    // =========================================================================
    // sceKernelIsNeoMode (618) - PS4 Pro detection
    // =========================================================================
    registerHandler(Syscall::SYS_sceKernelIsNeoMode, [](
        WeaR_Context&, WeaR_Memory&,
        uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)
    {
        return SyscallResult{1, true, ""};  // Always Pro mode
    });

    // =========================================================================
    // sceKernelGetCpuTemperature (621)
    // =========================================================================
    registerHandler(Syscall::SYS_sceKernelGetCpuTemperature, [](
        WeaR_Context&, WeaR_Memory& mem,
        uint64_t tempPtr, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)
    {
        if (tempPtr != 0) {
            mem.write<uint32_t>(tempPtr, 45);  // 45°C
        }
        return SyscallResult{0, true, ""};
    });

    // =========================================================================
    // sceKernelLoadStartModule (594)
    // =========================================================================
    registerHandler(Syscall::SYS_sceKernelLoadStartModule, [this](
        WeaR_Context&, WeaR_Memory& mem,
        uint64_t pathPtr, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)
    {
        std::string path;
        try {
            for (uint64_t i = 0; i < 256; ++i) {
                char c = static_cast<char>(mem.read<uint8_t>(pathPtr + i));
                if (c == '\0') break;
                path += c;
            }
        } catch (...) {
            return SyscallResult{-1, false, "EFAULT"};
        }

        log(std::format("LoadStartModule: {}", path));
        
        // Return fake module handle
        static int nextModuleId = 100;
        return SyscallResult{nextModuleId++, true, ""};
    });

    // =========================================================================
    // sceGnmSubmitCommandBuffers (591) - GPU command buffer submission
    // =========================================================================
    registerHandler(Syscall::SYS_sceGnmSubmitCommandBuffers, [this](
        WeaR_Context&, WeaR_Memory& mem,
        uint64_t count, uint64_t cmdBuffersPtr, uint64_t sizesPtr,
        uint64_t, uint64_t, uint64_t)
    {
        log(std::format("sceGnmSubmitCommandBuffers: count={}", count));
        
        int32_t result = getGnmDriver().handleSubmitCommandBuffers(
            static_cast<uint32_t>(count), cmdBuffersPtr, sizesPtr, mem);
        
        return SyscallResult{result, result == 0, ""};
    });

    // =========================================================================
    // sceGnmSubmitDone (614) - Signal GPU submission complete
    // =========================================================================
    registerHandler(Syscall::SYS_sceGnmSubmitDone, [this](
        WeaR_Context&, WeaR_Memory&,
        uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)
    {
        log("sceGnmSubmitDone");
        return SyscallResult{0, true, ""};
    });

    // =========================================================================
    // sceGnmGetGpuCoreClockFrequency (626)
    // =========================================================================
    registerHandler(Syscall::SYS_sceGnmGetGpuCoreClockFrequency, [](
        WeaR_Context&, WeaR_Memory&,
        uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)
    {
        // Return PS4 Pro GPU clock (911 MHz)
        return SyscallResult{911, true, ""};
    });
}

} // namespace WeaR

