#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace WeaR {

// PS4 PKG Magic: "\x7FCNT" (Big Endian: 0x7F434E54)
constexpr uint32_t PKG_MAGIC = 0x7F434E54;

#pragma pack(push, 1)
struct PkgHeader {
    uint32_t magic;             // 0x7F434E54
    uint32_t revision;          // PKG revision
    uint16_t type;              // PKG type
    uint16_t flags;             // PKG flags
    uint32_t entryCount;        // Number of entries
    uint16_t scEntryCount;      // SC entry count
    uint16_t entryCount2;       // Entry count 2
    uint32_t tableOffset;       // Entry table offset
    uint32_t entryDataSize;     // Entry data size
    uint64_t bodyOffset;        // Body offset
    uint64_t bodySize;          // Body size
    uint64_t contentOffset;     // Content offset
    uint64_t contentSize;       // Content size
    uint8_t contentId[36];      // Content ID string
    uint8_t padding[12];        // Padding
    uint32_t drmType;           // DRM type
    uint32_t contentType;       // Content type
    uint32_t contentFlags;      // Content flags
    uint32_t promoteSize;       // Promote size
    uint32_t versionDate;       // Version date
    uint32_t versionHash;       // Version hash
    uint32_t iroTag;            // IRO tag
    uint32_t ekcVersion;        // EKC version
    uint8_t reserved[0x60];     // Reserved
};

struct PkgEntry {
    uint32_t id;                // Entry ID
    uint32_t filenameOffset;    // Filename offset
    uint32_t flags1;            // Flags 1
    uint32_t flags2;            // Flags 2
    uint32_t dataOffset;        // Data offset
    uint32_t dataSize;          // Data size
    uint64_t padding;           // Padding
};
#pragma pack(pop)

// Common PKG entry IDs
constexpr uint32_t PKG_ENTRY_ID_EBOOT = 0x1000;        // eboot.bin
constexpr uint32_t PKG_ENTRY_ID_PARAM_SFO = 0x1001;    // param.sfo

struct PkgInfo {
    std::string contentId;
    uint32_t contentType;
    uint32_t entryCount;
    std::filesystem::path sourcePath;
};

class WeaR_PkgLoader {
public:
    WeaR_PkgLoader() = default;
    ~WeaR_PkgLoader() = default;

    // Load and validate a PKG file
    [[nodiscard]] std::expected<PkgInfo, std::string> loadPackage(const std::filesystem::path& pkgPath);

    // Extract eboot.bin to memory buffer
    [[nodiscard]] std::expected<std::vector<uint8_t>, std::string> extractEboot();

    // Extract any entry by ID
    [[nodiscard]] std::expected<std::vector<uint8_t>, std::string> extractEntry(uint32_t entryId);

    // Get package info
    [[nodiscard]] const PkgInfo& getInfo() const { return m_info; }

private:
    // Endian conversion helpers
    static uint16_t swapEndian16(uint16_t val);
    static uint32_t swapEndian32(uint32_t val);
    static uint64_t swapEndian64(uint64_t val);

    PkgHeader m_header{};
    std::vector<PkgEntry> m_entries;
    PkgInfo m_info{};
    std::filesystem::path m_pkgPath;
    bool m_loaded = false;
};

} // namespace WeaR
