#include "HLE/WeaR_Syscalls.h"
#include "HLE/FileSystem/WeaR_VFS.h"
#include "GUI/WeaR_Logger.h"

#include <iostream>
#include <format>
#include <cstring>

/**
 * @file WeaR_LibFS.cpp
 * @brief PS4 File System syscall HLE implementations
 * 
 * Handles sys_open, sys_read, sys_write, sys_close, sys_lseek, sys_fstat
 */

namespace WeaR {
namespace LibFS {

// =============================================================================
// SYSCALL NUMBERS (FreeBSD/PS4)
// =============================================================================

namespace Syscall {
    constexpr uint64_t SYS_read   = 3;
    constexpr uint64_t SYS_write  = 4;
    constexpr uint64_t SYS_open   = 5;
    constexpr uint64_t SYS_close  = 6;
    constexpr uint64_t SYS_fstat  = 189;
    constexpr uint64_t SYS_lseek  = 478;
    constexpr uint64_t SYS_stat   = 188;
    constexpr uint64_t SYS_mkdir  = 136;
    constexpr uint64_t SYS_unlink = 10;
    constexpr uint64_t SYS_getdents = 272;
}

// =============================================================================
// HELPER: Read C-string from memory
// =============================================================================

std::string readCString(WeaR_Memory& mem, uint64_t addr, size_t maxLen = 1024) {
    std::string result;
    result.reserve(maxLen);
    
    for (size_t i = 0; i < maxLen; ++i) {
        char c = static_cast<char>(mem.read<uint8_t>(addr + i));
        if (c == '\0') break;
        result += c;
    }
    
    return result;
}

// =============================================================================
// HLE HANDLERS
// =============================================================================

/**
 * @brief sys_open - Open or create a file
 * 
 * int open(const char* path, int flags, mode_t mode)
 * RDI = path pointer
 * RSI = flags
 * RDX = mode
 * Returns: file descriptor or error
 */
SyscallResult hle_sys_open(
    WeaR_Context& ctx, 
    WeaR_Memory& mem,
    uint64_t pathPtr, 
    uint64_t flags,
    uint64_t mode, 
    uint64_t, uint64_t, uint64_t)
{
    (void)ctx;
    
    if (pathPtr == 0) {
        return SyscallResult{PS4Error::SCE_ERROR_EINVAL, false, "null path"};
    }
    
    std::string path = readCString(mem, pathPtr);
    getLogger().log(std::format("sys_open: {} flags=0x{:X}", path, flags), LogLevel::Debug);
    
    int32_t fd = WeaR_VFS::get().openFile(path, static_cast<int>(flags), static_cast<int>(mode));
    
    if (fd < 0) {
        return SyscallResult{fd, false, std::format("open failed: {}", path)};
    }
    
    return SyscallResult{fd, true, ""};
}

/**
 * @brief sys_read - Read from file descriptor
 * 
 * ssize_t read(int fd, void* buf, size_t count)
 * RDI = fd
 * RSI = buffer pointer
 * RDX = count
 * Returns: bytes read or error
 */
SyscallResult hle_sys_read(
    WeaR_Context& ctx, 
    WeaR_Memory& mem,
    uint64_t fd, 
    uint64_t bufPtr,
    uint64_t count, 
    uint64_t, uint64_t, uint64_t)
{
    (void)ctx;
    
    if (bufPtr == 0 || count == 0) {
        return SyscallResult{0, true, ""};
    }
    
    // Allocate temporary buffer
    std::vector<uint8_t> buffer(count);
    
    int64_t bytesRead = WeaR_VFS::get().readFile(static_cast<int>(fd), buffer.data(), count);
    
    if (bytesRead < 0) {
        return SyscallResult{static_cast<int64_t>(bytesRead), false, "read failed"};
    }
    
    // Write to game memory
    for (int64_t i = 0; i < bytesRead; ++i) {
        mem.write<uint8_t>(bufPtr + i, buffer[i]);
    }
    
    return SyscallResult{bytesRead, true, ""};
}

/**
 * @brief sys_write - Write to file descriptor  
 * 
 * ssize_t write(int fd, const void* buf, size_t count)
 */
SyscallResult hle_sys_write(
    WeaR_Context& ctx, 
    WeaR_Memory& mem,
    uint64_t fd, 
    uint64_t bufPtr,
    uint64_t count, 
    uint64_t, uint64_t, uint64_t)
{
    (void)ctx;
    
    // Handle stdout/stderr specially
    if (fd == 1 || fd == 2) {
        std::string output;
        for (uint64_t i = 0; i < count && i < 4096; ++i) {
            char c = static_cast<char>(mem.read<uint8_t>(bufPtr + i));
            if (c == '\0') break;
            output += c;
        }
        getLogger().log(std::format("[fd{}] {}", fd, output), LogLevel::Info);
        return SyscallResult{static_cast<int64_t>(output.size()), true, ""};
    }
    
    // Regular file write
    if (bufPtr == 0 || count == 0) {
        return SyscallResult{0, true, ""};
    }
    
    std::vector<uint8_t> buffer(count);
    for (uint64_t i = 0; i < count; ++i) {
        buffer[i] = mem.read<uint8_t>(bufPtr + i);
    }
    
    int64_t bytesWritten = WeaR_VFS::get().writeFile(static_cast<int>(fd), buffer.data(), count);
    
    if (bytesWritten < 0) {
        return SyscallResult{bytesWritten, false, "write failed"};
    }
    
    return SyscallResult{bytesWritten, true, ""};
}

/**
 * @brief sys_close - Close file descriptor
 * 
 * int close(int fd)
 */
SyscallResult hle_sys_close(
    WeaR_Context& ctx, 
    WeaR_Memory& mem,
    uint64_t fd, 
    uint64_t, uint64_t,
    uint64_t, uint64_t, uint64_t)
{
    (void)ctx;
    (void)mem;
    
    int32_t result = WeaR_VFS::get().closeFile(static_cast<int>(fd));
    return SyscallResult{result, result == 0, ""};
}

/**
 * @brief sys_lseek - Reposition file offset
 * 
 * off_t lseek(int fd, off_t offset, int whence)
 */
SyscallResult hle_sys_lseek(
    WeaR_Context& ctx, 
    WeaR_Memory& mem,
    uint64_t fd, 
    uint64_t offset,
    uint64_t whence, 
    uint64_t, uint64_t, uint64_t)
{
    (void)ctx;
    (void)mem;
    
    int64_t newPos = WeaR_VFS::get().seekFile(
        static_cast<int>(fd), 
        static_cast<int64_t>(offset), 
        static_cast<int>(whence)
    );
    
    if (newPos < 0) {
        return SyscallResult{newPos, false, "lseek failed"};
    }
    
    return SyscallResult{newPos, true, ""};
}

/**
 * @brief sys_fstat - Get file status
 * 
 * int fstat(int fd, struct stat* buf)
 */
SyscallResult hle_sys_fstat(
    WeaR_Context& ctx, 
    WeaR_Memory& mem,
    uint64_t fd, 
    uint64_t statPtr,
    uint64_t, uint64_t, uint64_t, uint64_t)
{
    (void)ctx;
    
    if (statPtr == 0) {
        return SyscallResult{PS4Error::SCE_ERROR_EINVAL, false, "null stat ptr"};
    }
    
    PS4Stat stat{};
    int32_t result = WeaR_VFS::get().statFile(static_cast<int>(fd), stat);
    
    if (result != PS4Error::SCE_OK) {
        return SyscallResult{result, false, "fstat failed"};
    }
    
    // Write stat structure to memory
    const uint8_t* statBytes = reinterpret_cast<const uint8_t*>(&stat);
    for (size_t i = 0; i < sizeof(PS4Stat); ++i) {
        mem.write<uint8_t>(statPtr + i, statBytes[i]);
    }
    
    return SyscallResult{0, true, ""};
}

/**
 * @brief sys_stat - Stat by path
 * 
 * int stat(const char* path, struct stat* buf)
 */
SyscallResult hle_sys_stat(
    WeaR_Context& ctx, 
    WeaR_Memory& mem,
    uint64_t pathPtr, 
    uint64_t statPtr,
    uint64_t, uint64_t, uint64_t, uint64_t)
{
    (void)ctx;
    
    if (pathPtr == 0 || statPtr == 0) {
        return SyscallResult{PS4Error::SCE_ERROR_EINVAL, false, "null pointer"};
    }
    
    std::string path = readCString(mem, pathPtr);
    
    PS4Stat stat{};
    int32_t result = WeaR_VFS::get().statPath(path, stat);
    
    if (result != PS4Error::SCE_OK) {
        return SyscallResult{result, false, std::format("stat failed: {}", path)};
    }
    
    const uint8_t* statBytes = reinterpret_cast<const uint8_t*>(&stat);
    for (size_t i = 0; i < sizeof(PS4Stat); ++i) {
        mem.write<uint8_t>(statPtr + i, statBytes[i]);
    }
    
    return SyscallResult{0, true, ""};
}

// =============================================================================
// REGISTRATION
// =============================================================================

/**
 * @brief Register file system syscall handlers with dispatcher
 */
void registerLibFSHandlers(WeaR_Syscalls& dispatcher) {
    dispatcher.registerHandler(Syscall::SYS_open, hle_sys_open);
    dispatcher.registerHandler(Syscall::SYS_read, hle_sys_read);
    dispatcher.registerHandler(Syscall::SYS_write, hle_sys_write);
    dispatcher.registerHandler(Syscall::SYS_close, hle_sys_close);
    dispatcher.registerHandler(Syscall::SYS_lseek, hle_sys_lseek);
    dispatcher.registerHandler(Syscall::SYS_fstat, hle_sys_fstat);
    dispatcher.registerHandler(Syscall::SYS_stat, hle_sys_stat);
    
    std::cout << "[HLE] libFS handlers registered\n";
}

} // namespace LibFS
} // namespace WeaR
