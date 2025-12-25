#pragma once

/**
 * @file WeaR_VFS.h
 * @brief Virtual File System for PS4 path translation
 * 
 * Maps PS4 internal paths (e.g., /app0/...) to local Windows directories.
 * Provides sandboxed file I/O with proper error handling.
 */

#include <string>
#include <map>
#include <unordered_map>
#include <mutex>
#include <filesystem>
#include <fstream>
#include <memory>
#include <cstdint>

namespace WeaR {

// =============================================================================
// PS4 ERROR CODES
// =============================================================================

namespace PS4Error {
    constexpr int32_t SCE_OK             = 0;
    constexpr int32_t SCE_ERROR_ENOENT   = 0x80020002;  // No such file
    constexpr int32_t SCE_ERROR_EACCES   = 0x80020013;  // Permission denied
    constexpr int32_t SCE_ERROR_EEXIST   = 0x80020011;  // File exists
    constexpr int32_t SCE_ERROR_EBADF    = 0x80020009;  // Bad file descriptor
    constexpr int32_t SCE_ERROR_EINVAL   = 0x80020022;  // Invalid argument
    constexpr int32_t SCE_ERROR_ENOSPC   = 0x80020028;  // No space left
    constexpr int32_t SCE_ERROR_ENOMEM   = 0x80020012;  // Out of memory
}

// =============================================================================
// FILE OPEN FLAGS (PS4/POSIX compatible)
// =============================================================================

namespace OpenFlags {
    constexpr int O_RDONLY   = 0x0000;
    constexpr int O_WRONLY   = 0x0001;
    constexpr int O_RDWR     = 0x0002;
    constexpr int O_CREAT    = 0x0200;
    constexpr int O_TRUNC    = 0x0400;
    constexpr int O_APPEND   = 0x0008;
    constexpr int O_NONBLOCK = 0x0004;
    constexpr int O_DIRECTORY = 0x00020000;
}

// =============================================================================
// FILE STAT STRUCTURE
// =============================================================================

#pragma pack(push, 1)
struct PS4Stat {
    uint32_t st_dev;
    uint32_t st_ino;
    uint16_t st_mode;
    uint16_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint32_t st_rdev;
    int64_t  st_size;
    int64_t  st_atime;
    int64_t  st_mtime;
    int64_t  st_ctime;
    int64_t  st_blksize;
    int64_t  st_blocks;
};
#pragma pack(pop)

// =============================================================================
// OPEN FILE HANDLE
// =============================================================================

struct FileHandle {
    std::unique_ptr<std::fstream> stream;
    std::filesystem::path hostPath;
    int flags = 0;
    bool isDirectory = false;
};

// =============================================================================
// VFS CLASS
// =============================================================================

/**
 * @brief Virtual File System singleton
 */
class WeaR_VFS {
public:
    /**
     * @brief Get singleton instance
     */
    static WeaR_VFS& get();

    // =========================================================================
    // MOUNT MANAGEMENT
    // =========================================================================
    
    /**
     * @brief Mount a virtual path to a host directory
     * @param virtualPath PS4 path prefix (e.g., "/app0")
     * @param hostPath Windows directory path
     * @return true if mounted successfully
     */
    bool mount(const std::string& virtualPath, const std::string& hostPath);

    /**
     * @brief Unmount a virtual path
     */
    void unmount(const std::string& virtualPath);

    /**
     * @brief Clear all mount points
     */
    void clearMounts();

    /**
     * @brief Check if path is mounted
     */
    [[nodiscard]] bool isMounted(const std::string& virtualPath) const;

    // =========================================================================
    // PATH RESOLUTION
    // =========================================================================

    /**
     * @brief Resolve PS4 path to Windows host path
     * @param ps4Path Path like "/app0/data/file.dat"
     * @return Resolved Windows path, or empty if not found
     */
    [[nodiscard]] std::filesystem::path resolvePath(const std::string& ps4Path) const;

    /**
     * @brief Normalize path separators
     */
    [[nodiscard]] static std::string normalizePath(const std::string& path);

    // =========================================================================
    // FILE OPERATIONS
    // =========================================================================

    /**
     * @brief Open a file
     * @return File descriptor (positive) or error code (negative)
     */
    [[nodiscard]] int32_t openFile(const std::string& ps4Path, int flags, int mode);

    /**
     * @brief Close a file
     * @return 0 on success, error code on failure
     */
    [[nodiscard]] int32_t closeFile(int fd);

    /**
     * @brief Read from file
     * @return Bytes read, or error code (negative)
     */
    [[nodiscard]] int64_t readFile(int fd, void* buffer, size_t size);

    /**
     * @brief Write to file
     * @return Bytes written, or error code (negative)
     */
    [[nodiscard]] int64_t writeFile(int fd, const void* buffer, size_t size);

    /**
     * @brief Seek in file
     * @return New position, or error code (negative)
     */
    [[nodiscard]] int64_t seekFile(int fd, int64_t offset, int whence);

    /**
     * @brief Get file statistics
     * @return 0 on success, error code on failure
     */
    [[nodiscard]] int32_t statFile(int fd, PS4Stat& stat);

    /**
     * @brief Stat by path (without opening)
     */
    [[nodiscard]] int32_t statPath(const std::string& ps4Path, PS4Stat& stat);

    // =========================================================================
    // DIRECTORY OPERATIONS
    // =========================================================================

    /**
     * @brief Open directory for reading
     */
    [[nodiscard]] int32_t openDirectory(const std::string& ps4Path);

    /**
     * @brief Check if file exists
     */
    [[nodiscard]] bool fileExists(const std::string& ps4Path) const;

    // =========================================================================
    // STATISTICS
    // =========================================================================

    [[nodiscard]] size_t getMountCount() const;
    [[nodiscard]] size_t getOpenFileCount() const;
    [[nodiscard]] uint64_t getTotalBytesRead() const { return m_totalBytesRead; }
    [[nodiscard]] uint64_t getTotalBytesWritten() const { return m_totalBytesWritten; }

private:
    WeaR_VFS();
    ~WeaR_VFS();

    [[nodiscard]] bool isPathSafe(const std::filesystem::path& path) const;
    [[nodiscard]] int allocateFd();

    std::map<std::string, std::filesystem::path> m_mountPoints;
    std::unordered_map<int, std::unique_ptr<FileHandle>> m_openFiles;
    mutable std::mutex m_mutex;
    
    int m_nextFd = 10;  // Start after stdin/stdout/stderr
    uint64_t m_totalBytesRead = 0;
    uint64_t m_totalBytesWritten = 0;
};

} // namespace WeaR
