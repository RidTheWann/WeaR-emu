#include "WeaR_VFS.h"

#include <iostream>
#include <format>
#include <algorithm>

namespace WeaR {

// =============================================================================
// SINGLETON INSTANCE
// =============================================================================

WeaR_VFS& WeaR_VFS::get() {
    static WeaR_VFS instance;
    return instance;
}

// =============================================================================
// CONSTRUCTOR / DESTRUCTOR
// =============================================================================

WeaR_VFS::WeaR_VFS() {
    std::cout << "[VFS] Virtual File System initialized\n";
}

WeaR_VFS::~WeaR_VFS() {
    // Close all open files
    std::lock_guard<std::mutex> lock(m_mutex);
    m_openFiles.clear();
    m_mountPoints.clear();
}

// =============================================================================
// MOUNT MANAGEMENT
// =============================================================================

bool WeaR_VFS::mount(const std::string& virtualPath, const std::string& hostPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::filesystem::path host = hostPath;
    if (!std::filesystem::exists(host)) {
        std::cerr << std::format("[VFS] Mount failed: host path does not exist: {}\n", hostPath);
        return false;
    }
    
    std::string normalized = normalizePath(virtualPath);
    m_mountPoints[normalized] = std::filesystem::canonical(host);
    
    std::cout << std::format("[VFS] Mounted {} -> {}\n", normalized, host.string());
    return true;
}

void WeaR_VFS::unmount(const std::string& virtualPath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string normalized = normalizePath(virtualPath);
    m_mountPoints.erase(normalized);
}

void WeaR_VFS::clearMounts() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mountPoints.clear();
}

bool WeaR_VFS::isMounted(const std::string& virtualPath) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string normalized = normalizePath(virtualPath);
    return m_mountPoints.find(normalized) != m_mountPoints.end();
}

// =============================================================================
// PATH RESOLUTION
// =============================================================================

std::string WeaR_VFS::normalizePath(const std::string& path) {
    std::string result = path;
    // Convert backslashes to forward slashes
    std::replace(result.begin(), result.end(), '\\', '/');
    // Remove trailing slash
    while (!result.empty() && result.back() == '/') {
        result.pop_back();
    }
    // Ensure leading slash
    if (result.empty() || result[0] != '/') {
        result = "/" + result;
    }
    return result;
}

std::filesystem::path WeaR_VFS::resolvePath(const std::string& ps4Path) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::string normalized = normalizePath(ps4Path);
    
    // Find best matching mount point
    std::string bestMatch;
    std::filesystem::path bestHost;
    
    for (const auto& [mountPoint, hostPath] : m_mountPoints) {
        if (normalized.find(mountPoint) == 0) {
            if (mountPoint.length() > bestMatch.length()) {
                bestMatch = mountPoint;
                bestHost = hostPath;
            }
        }
    }
    
    if (bestMatch.empty()) {
        return {};  // Not mounted
    }
    
    // Get relative path after mount point
    std::string relativePath = normalized.substr(bestMatch.length());
    if (!relativePath.empty() && relativePath[0] == '/') {
        relativePath = relativePath.substr(1);
    }
    
    std::filesystem::path resolved = bestHost / relativePath;
    
    // Security check: ensure resolved path is within mount point
    if (!isPathSafe(resolved)) {
        std::cerr << std::format("[VFS] Security: path escape attempt: {}\n", ps4Path);
        return {};
    }
    
    return resolved;
}

bool WeaR_VFS::isPathSafe(const std::filesystem::path& path) const {
    // Check if path tries to escape mount points
    try {
        std::filesystem::path canonical = std::filesystem::weakly_canonical(path);
        
        for (const auto& [mount, host] : m_mountPoints) {
            std::filesystem::path hostCanonical = std::filesystem::canonical(host);
            // Check if path starts with host path
            auto [end, _] = std::mismatch(
                hostCanonical.begin(), hostCanonical.end(),
                canonical.begin(), canonical.end()
            );
            if (end == hostCanonical.end()) {
                return true;  // Path is within this mount point
            }
        }
    } catch (...) {
        return false;
    }
    return false;
}

// =============================================================================
// FILE OPERATIONS
// =============================================================================

int WeaR_VFS::allocateFd() {
    return m_nextFd++;
}

int32_t WeaR_VFS::openFile(const std::string& ps4Path, int flags, int mode) {
    (void)mode;  // Ignore mode for now
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::filesystem::path hostPath = resolvePath(ps4Path);
    if (hostPath.empty()) {
        std::cerr << std::format("[VFS] Open failed: cannot resolve path: {}\n", ps4Path);
        return PS4Error::SCE_ERROR_ENOENT;
    }
    
    // Check if it's a directory open
    if (flags & OpenFlags::O_DIRECTORY) {
        if (!std::filesystem::is_directory(hostPath)) {
            return PS4Error::SCE_ERROR_ENOENT;
        }
        
        auto handle = std::make_unique<FileHandle>();
        handle->hostPath = hostPath;
        handle->flags = flags;
        handle->isDirectory = true;
        
        int fd = allocateFd();
        m_openFiles[fd] = std::move(handle);
        
        std::cout << std::format("[VFS] Opened directory: {} -> fd={}\n", ps4Path, fd);
        return fd;
    }
    
    // Regular file
    std::ios_base::openmode openMode = std::ios::binary;
    
    if ((flags & OpenFlags::O_RDWR) == OpenFlags::O_RDWR) {
        openMode |= std::ios::in | std::ios::out;
    } else if (flags & OpenFlags::O_WRONLY) {
        openMode |= std::ios::out;
    } else {
        openMode |= std::ios::in;  // Default to read
    }
    
    if (flags & OpenFlags::O_CREAT) {
        openMode |= std::ios::out;
    }
    if (flags & OpenFlags::O_TRUNC) {
        openMode |= std::ios::trunc;
    }
    if (flags & OpenFlags::O_APPEND) {
        openMode |= std::ios::app;
    }
    
    // Check if file exists for read-only
    if (!(flags & OpenFlags::O_CREAT) && !std::filesystem::exists(hostPath)) {
        return PS4Error::SCE_ERROR_ENOENT;
    }
    
    auto stream = std::make_unique<std::fstream>(hostPath, openMode);
    if (!stream->is_open()) {
        return PS4Error::SCE_ERROR_EACCES;
    }
    
    auto handle = std::make_unique<FileHandle>();
    handle->stream = std::move(stream);
    handle->hostPath = hostPath;
    handle->flags = flags;
    handle->isDirectory = false;
    
    int fd = allocateFd();
    m_openFiles[fd] = std::move(handle);
    
    std::cout << std::format("[VFS] Opened: {} -> fd={}\n", ps4Path, fd);
    return fd;
}

int32_t WeaR_VFS::closeFile(int fd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_openFiles.find(fd);
    if (it == m_openFiles.end()) {
        return PS4Error::SCE_ERROR_EBADF;
    }
    
    m_openFiles.erase(it);
    return PS4Error::SCE_OK;
}

int64_t WeaR_VFS::readFile(int fd, void* buffer, size_t size) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_openFiles.find(fd);
    if (it == m_openFiles.end()) {
        return PS4Error::SCE_ERROR_EBADF;
    }
    
    FileHandle* handle = it->second.get();
    if (handle->isDirectory || !handle->stream) {
        return PS4Error::SCE_ERROR_EBADF;
    }
    
    handle->stream->read(static_cast<char*>(buffer), size);
    std::streamsize bytesRead = handle->stream->gcount();
    
    m_totalBytesRead += bytesRead;
    return static_cast<int64_t>(bytesRead);
}

int64_t WeaR_VFS::writeFile(int fd, const void* buffer, size_t size) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_openFiles.find(fd);
    if (it == m_openFiles.end()) {
        return PS4Error::SCE_ERROR_EBADF;
    }
    
    FileHandle* handle = it->second.get();
    if (handle->isDirectory || !handle->stream) {
        return PS4Error::SCE_ERROR_EBADF;
    }
    
    handle->stream->write(static_cast<const char*>(buffer), size);
    if (handle->stream->fail()) {
        return PS4Error::SCE_ERROR_ENOSPC;
    }
    
    m_totalBytesWritten += size;
    return static_cast<int64_t>(size);
}

int64_t WeaR_VFS::seekFile(int fd, int64_t offset, int whence) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_openFiles.find(fd);
    if (it == m_openFiles.end()) {
        return PS4Error::SCE_ERROR_EBADF;
    }
    
    FileHandle* handle = it->second.get();
    if (handle->isDirectory || !handle->stream) {
        return PS4Error::SCE_ERROR_EBADF;
    }
    
    std::ios_base::seekdir dir;
    switch (whence) {
        case 0: dir = std::ios::beg; break;  // SEEK_SET
        case 1: dir = std::ios::cur; break;  // SEEK_CUR
        case 2: dir = std::ios::end; break;  // SEEK_END
        default: return PS4Error::SCE_ERROR_EINVAL;
    }
    
    handle->stream->seekg(offset, dir);
    handle->stream->seekp(offset, dir);
    
    return static_cast<int64_t>(handle->stream->tellg());
}

int32_t WeaR_VFS::statFile(int fd, PS4Stat& stat) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_openFiles.find(fd);
    if (it == m_openFiles.end()) {
        return PS4Error::SCE_ERROR_EBADF;
    }
    
    FileHandle* handle = it->second.get();
    
    std::memset(&stat, 0, sizeof(stat));
    
    try {
        if (handle->isDirectory) {
            stat.st_mode = 0040755;  // Directory
            stat.st_size = 0;
        } else {
            stat.st_mode = 0100644;  // Regular file
            stat.st_size = std::filesystem::file_size(handle->hostPath);
        }
        
        auto ftime = std::filesystem::last_write_time(handle->hostPath);
        // MSVC workaround: convert file_time to system time
        auto duration = ftime.time_since_epoch();
        auto sys_duration = std::chrono::duration_cast<std::chrono::seconds>(duration);
        stat.st_mtime = sys_duration.count();
        stat.st_atime = stat.st_mtime;
        stat.st_ctime = stat.st_mtime;
        stat.st_blksize = 4096;
        stat.st_blocks = (stat.st_size + 511) / 512;
        stat.st_nlink = 1;
    } catch (...) {
        return PS4Error::SCE_ERROR_ENOENT;
    }
    
    return PS4Error::SCE_OK;
}

int32_t WeaR_VFS::statPath(const std::string& ps4Path, PS4Stat& stat) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::filesystem::path hostPath = resolvePath(ps4Path);
    if (hostPath.empty() || !std::filesystem::exists(hostPath)) {
        return PS4Error::SCE_ERROR_ENOENT;
    }
    
    std::memset(&stat, 0, sizeof(stat));
    
    try {
        if (std::filesystem::is_directory(hostPath)) {
            stat.st_mode = 0040755;
            stat.st_size = 0;
        } else {
            stat.st_mode = 0100644;
            stat.st_size = std::filesystem::file_size(hostPath);
        }
        
        auto ftime = std::filesystem::last_write_time(hostPath);
        // MSVC workaround: convert file_time to system time
        auto duration = ftime.time_since_epoch();
        auto sys_duration = std::chrono::duration_cast<std::chrono::seconds>(duration);
        stat.st_mtime = sys_duration.count();
        stat.st_atime = stat.st_mtime;
        stat.st_ctime = stat.st_mtime;
        stat.st_blksize = 4096;
        stat.st_blocks = (stat.st_size + 511) / 512;
        stat.st_nlink = 1;
    } catch (...) {
        return PS4Error::SCE_ERROR_ENOENT;
    }
    
    return PS4Error::SCE_OK;
}

// =============================================================================
// DIRECTORY OPERATIONS
// =============================================================================

int32_t WeaR_VFS::openDirectory(const std::string& ps4Path) {
    return openFile(ps4Path, OpenFlags::O_RDONLY | OpenFlags::O_DIRECTORY, 0);
}

bool WeaR_VFS::fileExists(const std::string& ps4Path) const {
    std::filesystem::path hostPath = resolvePath(ps4Path);
    return !hostPath.empty() && std::filesystem::exists(hostPath);
}

// =============================================================================
// STATISTICS
// =============================================================================

size_t WeaR_VFS::getMountCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_mountPoints.size();
}

size_t WeaR_VFS::getOpenFileCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_openFiles.size();
}

} // namespace WeaR
