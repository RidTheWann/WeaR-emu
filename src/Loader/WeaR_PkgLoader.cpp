#include "WeaR_PkgLoader.h"
#include "GUI/WeaR_Logger.h"
#include <fstream>
#include <cstring>
#include <format>
#include <algorithm>

namespace WeaR {

// Helper to log to GUI
static void logInfo(const std::string& msg) {
    getLogger().log(msg, LogLevel::Info);
}

static void logWarning(const std::string& msg) {
    getLogger().log(msg, LogLevel::Warning);
}

static void logError(const std::string& msg) {
    getLogger().log(msg, LogLevel::Error);
}

uint16_t WeaR_PkgLoader::swapEndian16(uint16_t val) {
    return ((val & 0xFF00) >> 8) | ((val & 0x00FF) << 8);
}

uint32_t WeaR_PkgLoader::swapEndian32(uint32_t val) {
    return ((val & 0xFF000000) >> 24) |
           ((val & 0x00FF0000) >> 8) |
           ((val & 0x0000FF00) << 8) |
           ((val & 0x000000FF) << 24);
}

uint64_t WeaR_PkgLoader::swapEndian64(uint64_t val) {
    return ((val & 0xFF00000000000000ULL) >> 56) |
           ((val & 0x00FF000000000000ULL) >> 40) |
           ((val & 0x0000FF0000000000ULL) >> 24) |
           ((val & 0x000000FF00000000ULL) >> 8) |
           ((val & 0x00000000FF000000ULL) << 8) |
           ((val & 0x0000000000FF0000ULL) << 24) |
           ((val & 0x000000000000FF00ULL) << 40) |
           ((val & 0x00000000000000FFULL) << 56);
}

std::expected<PkgInfo, std::string> WeaR_PkgLoader::loadPackage(const std::filesystem::path& pkgPath) {
    m_loaded = false;
    m_pkgPath = pkgPath;
    m_entries.clear();

    logInfo("[PKG] ============ X-RAY LOADER ============");
    logInfo(std::format("[PKG] Opening file: {}", pkgPath.string()));

    // Get file size
    std::error_code ec;
    auto fileSize = std::filesystem::file_size(pkgPath, ec);
    if (ec) {
        logError(std::format("[PKG] Cannot get file size: {}", ec.message()));
        return std::unexpected(std::format("Cannot access PKG file: {}", ec.message()));
    }
    logInfo(std::format("[PKG] File size: {} bytes ({} MB)", fileSize, fileSize / 1024 / 1024));

    // Open file
    std::ifstream file(pkgPath, std::ios::binary);
    if (!file) {
        logError("[PKG] Failed to open file!");
        return std::unexpected(std::format("Failed to open PKG file: {}", pkgPath.string()));
    }
    logInfo("[PKG] File opened successfully");

    // Read header
    logInfo(std::format("[PKG] Reading header ({} bytes)...", sizeof(PkgHeader)));
    file.read(reinterpret_cast<char*>(&m_header), sizeof(PkgHeader));
    if (!file) {
        logError("[PKG] Failed to read header (file too small?)");
        return std::unexpected("Failed to read PKG header - file may be corrupted or too small");
    }

    // Validate magic (PKG uses Big Endian)
    uint32_t magic = swapEndian32(m_header.magic);
    logInfo(std::format("[PKG] Magic Check: Read 0x{:08X}, Expected 0x{:08X}", magic, PKG_MAGIC));
    if (magic != PKG_MAGIC) {
        logError("[PKG] Magic mismatch! This is NOT a valid PS4 PKG file.");
        return std::unexpected(std::format("Invalid PKG magic: 0x{:08X} (expected 0x{:08X})", 
                                           magic, PKG_MAGIC));
    }
    logInfo("[PKG] Magic OK - Valid PS4 PKG signature");

    // Convert header fields from Big Endian
    m_header.revision = swapEndian32(m_header.revision);
    m_header.type = swapEndian16(m_header.type);
    m_header.flags = swapEndian16(m_header.flags);
    m_header.entryCount = swapEndian32(m_header.entryCount);
    m_header.tableOffset = swapEndian32(m_header.tableOffset);
    m_header.entryDataSize = swapEndian32(m_header.entryDataSize);
    m_header.bodyOffset = swapEndian64(m_header.bodyOffset);
    m_header.bodySize = swapEndian64(m_header.bodySize);
    m_header.contentOffset = swapEndian64(m_header.contentOffset);
    m_header.contentSize = swapEndian64(m_header.contentSize);
    m_header.drmType = swapEndian32(m_header.drmType);
    m_header.contentType = swapEndian32(m_header.contentType);

    logInfo(std::format("[PKG] Header: Rev={}, Type={}, Flags=0x{:04X}", 
            m_header.revision, m_header.type, m_header.flags));
    logInfo(std::format("[PKG] Entries={}, TableOffset=0x{:08X}", 
            m_header.entryCount, m_header.tableOffset));
    logInfo(std::format("[PKG] DRM={}, ContentType={}", 
            m_header.drmType, m_header.contentType));

    // Read entry table
    logInfo(std::format("[PKG] Reading entry table at offset 0x{:08X}...", m_header.tableOffset));
    file.seekg(m_header.tableOffset, std::ios::beg);
    m_entries.resize(m_header.entryCount);
    
    for (uint32_t i = 0; i < m_header.entryCount; ++i) {
        file.read(reinterpret_cast<char*>(&m_entries[i]), sizeof(PkgEntry));
        if (!file) {
            logError(std::format("[PKG] Failed to read entry {}/{}", i, m_header.entryCount));
            return std::unexpected(std::format("Failed to read entry {} of {}", i, m_header.entryCount));
        }
        
        // Convert from Big Endian
        m_entries[i].id = swapEndian32(m_entries[i].id);
        m_entries[i].filenameOffset = swapEndian32(m_entries[i].filenameOffset);
        m_entries[i].flags1 = swapEndian32(m_entries[i].flags1);
        m_entries[i].flags2 = swapEndian32(m_entries[i].flags2);
        m_entries[i].dataOffset = swapEndian32(m_entries[i].dataOffset);
        m_entries[i].dataSize = swapEndian32(m_entries[i].dataSize);
    }
    logInfo(std::format("[PKG] Read {} entries successfully", m_header.entryCount));

    // Search for EBOOT.BIN
    logInfo(std::format("[PKG] Searching for EBOOT.BIN (Entry ID 0x{:04X})...", PKG_ENTRY_ID_EBOOT));
    bool ebootFound = false;
    for (const auto& entry : m_entries) {
        if (entry.id == PKG_ENTRY_ID_EBOOT) {
            ebootFound = true;
            logInfo(std::format("[PKG] EBOOT.BIN FOUND! Offset=0x{:08X}, Size={} bytes", 
                    entry.dataOffset, entry.dataSize));
            break;
        }
    }
    if (!ebootFound) {
        logWarning("[PKG] EBOOT.BIN not found in entry table!");
        std::string ids = "[PKG] Available IDs: ";
        for (size_t i = 0; i < std::min(m_entries.size(), size_t(10)); ++i) {
            ids += std::format("0x{:04X} ", m_entries[i].id);
        }
        if (m_entries.size() > 10) ids += "...";
        logInfo(ids);
    }

    // Populate info
    m_info.contentId = std::string(reinterpret_cast<char*>(m_header.contentId), 36);
    // Trim null characters safely
    auto nullPos = m_info.contentId.find('\0');
    if (nullPos != std::string::npos) {
        m_info.contentId.erase(nullPos);
    }
    m_info.contentType = m_header.contentType;
    m_info.entryCount = m_header.entryCount;
    m_info.sourcePath = pkgPath;

    logInfo(std::format("[PKG] Content ID: {}", m_info.contentId));
    logInfo("[PKG] ============ LOAD COMPLETE ============");

    m_loaded = true;
    return m_info;
}

std::expected<std::vector<uint8_t>, std::string> WeaR_PkgLoader::extractEboot() {
    // Try standard EBOOT ID first
    auto result = extractEntry(PKG_ENTRY_ID_EBOOT);
    
    if (result.has_value()) {
        return result;
    }
    
    // SMART FALLBACK: Standard EBOOT not found (common in PS2 Classics, remasters, etc.)
    logWarning("[PKG] Standard EBOOT (0x1000) not found - using SMART FALLBACK");
    
    if (!m_loaded || m_entries.empty()) {
        return std::unexpected("No PKG loaded or no entries found");
    }
    
    // Get file size for validation
    std::error_code ec;
    auto fileSize = std::filesystem::file_size(m_pkgPath, ec);
    if (ec) {
        return std::unexpected(std::format("Cannot get PKG file size: {}", ec.message()));
    }
    
    // Find the LARGEST VALID entry (offset must be within file bounds)
    const PkgEntry* largestEntry = nullptr;
    uint64_t maxSize = 0;
    
    for (const auto& entry : m_entries) {
        // CRITICAL: Skip entries with invalid offsets (prevents underflow)
        if (entry.dataOffset >= fileSize) {
            logWarning(std::format("[PKG] Skipping entry 0x{:08X}: offset {} >= fileSize {}",
                                  entry.id, entry.dataOffset, fileSize));
            continue;
        }
        
        // Calculate max readable size for this entry
        uint64_t maxReadable = fileSize - entry.dataOffset;
        uint64_t effectiveSize = std::min(static_cast<uint64_t>(entry.dataSize), maxReadable);
        
        // Only consider entries with non-zero size
        if (effectiveSize > 0 && effectiveSize > maxSize) {
            maxSize = effectiveSize;
            largestEntry = &entry;
        }
    }
    
    if (!largestEntry) {
        return std::unexpected("No valid entries found in PKG (all offsets invalid)");
    }
    
    // Log fallback decision
    logWarning(std::format("[PKG] FALLBACK: Loading largest valid entry (ID: 0x{:08X}, Size: {} MB)", 
                          largestEntry->id, maxSize / 1024 / 1024));
    
    // Extract the largest entry
    return extractEntry(largestEntry->id);
}

std::expected<std::vector<uint8_t>, std::string> WeaR_PkgLoader::extractEntry(uint32_t entryId) {
    if (!m_loaded) {
        return std::unexpected("No PKG loaded");
    }

    // Find entry
    const PkgEntry* targetEntry = nullptr;
    for (const auto& entry : m_entries) {
        if (entry.id == entryId) {
            targetEntry = &entry;
            break;
        }
    }

    if (!targetEntry) {
        return std::unexpected(std::format("Entry ID 0x{:08X} not found in PKG", entryId));
    }

    // Get file size for validation
    std::error_code ec;
    auto fileSize = std::filesystem::file_size(m_pkgPath, ec);
    if (ec) {
        return std::unexpected(std::format("Cannot get PKG file size: {}", ec.message()));
    }

    // STEP 1: VALIDATE OFFSET FIRST (prevents underflow)
    if (targetEntry->dataOffset >= fileSize) {
        logError(std::format("[PKG] INVALID OFFSET! Entry 0x{:08X}: offset={} >= fileSize={}",
                            entryId, targetEntry->dataOffset, fileSize));
        return std::unexpected(std::format("Entry offset ({}) is beyond file size ({})",
                                          targetEntry->dataOffset, fileSize));
    }

    // STEP 2: SAFE SIZE CALCULATION (guaranteed no underflow)
    uint64_t maxReadable = fileSize - targetEntry->dataOffset;  // Safe: offset < fileSize
    uint64_t requestedSize = targetEntry->dataSize;
    uint64_t finalSize = requestedSize;

    // Validate and sanitize size
    if (requestedSize == 0) {
        logError(std::format("[PKG] Entry 0x{:08X} has zero size!", entryId));
        return std::unexpected("Entry has zero size");
    }

    if (requestedSize > maxReadable) {
        logWarning(std::format("[PKG] Size overflow detected for entry 0x{:08X}", entryId));
        logWarning(std::format("[PKG] Requested: {} bytes, Max readable: {} bytes", 
                              requestedSize, maxReadable));
        logWarning(std::format("[PKG] Sanitizing size: {} -> {} bytes", requestedSize, maxReadable));
        finalSize = maxReadable;
    }

    // Prevent absurd allocations (>2GB is suspicious)
    if (finalSize > 2ULL * 1024 * 1024 * 1024) {
        return std::unexpected(std::format("Entry size too large: {} MB (possible corruption)",
                                          finalSize / 1024 / 1024));
    }

    logInfo(std::format("[PKG] Extracting entry 0x{:08X}: offset={}, size={} MB",
                        entryId, targetEntry->dataOffset, finalSize / 1024 / 1024));

    // Open file and extract
    std::ifstream file(m_pkgPath, std::ios::binary);
    if (!file) {
        return std::unexpected("Failed to reopen PKG file");
    }

    // Seek to entry data
    file.seekg(targetEntry->dataOffset, std::ios::beg);
    if (!file) {
        return std::unexpected(std::format("Failed to seek to offset {}", targetEntry->dataOffset));
    }
    
    // Allocate and read data with final sanitized size
    std::vector<uint8_t> data(static_cast<size_t>(finalSize));
    file.read(reinterpret_cast<char*>(data.data()), finalSize);
    
    if (!file && !file.eof()) {
        return std::unexpected(std::format("Failed to read entry data (requested: {} bytes, read: {} bytes)",
                                          finalSize, file.gcount()));
    }

    // If we hit EOF early, resize to actual bytes read
    if (file.eof()) {
        auto actualRead = file.gcount();
        if (actualRead < static_cast<std::streamsize>(finalSize)) {
            logWarning(std::format("[PKG] Hit EOF early: requested {} bytes, got {} bytes",
                                  finalSize, actualRead));
            data.resize(actualRead);
        }
    }

    logInfo(std::format("[PKG] ✓ Successfully extracted {} bytes", data.size()));
    return data;
}

} // namespace WeaR
