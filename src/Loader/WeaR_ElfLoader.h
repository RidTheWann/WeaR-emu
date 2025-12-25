#pragma once

/**
 * @file WeaR_ElfLoader.h
 * @brief PS4 ELF64 Executable Loader
 * 
 * Parses and loads PS4 ELF executables into emulator memory.
 * Supports standard ELF64 format with FreeBSD/PS4 ABI.
 */

#include "Core/WeaR_Memory.h"

#include <string>
#include <vector>
#include <cstdint>
#include <expected>
#include <filesystem>

namespace WeaR {

// =============================================================================
// ELF64 STRUCTURES (from ELF specification)
// =============================================================================

namespace Elf64 {

// ELF Magic
constexpr uint8_t MAGIC[4] = { 0x7F, 'E', 'L', 'F' };

// ELF Class
constexpr uint8_t CLASS_64 = 2;

// ELF Data encoding
constexpr uint8_t DATA_LSB = 1;  // Little-endian

// ELF OS ABI
constexpr uint8_t OSABI_FREEBSD = 9;
constexpr uint8_t OSABI_PS4 = 9;  // PS4 uses FreeBSD ABI

// ELF Type
constexpr uint16_t ET_EXEC = 2;  // Executable
constexpr uint16_t ET_DYN = 3;   // Shared object (also used by PS4)

// Machine type
constexpr uint16_t EM_X86_64 = 62;

// Program header types
constexpr uint32_t PT_NULL = 0;
constexpr uint32_t PT_LOAD = 1;       // Loadable segment
constexpr uint32_t PT_DYNAMIC = 2;    // Dynamic linking info
constexpr uint32_t PT_INTERP = 3;     // Interpreter path
constexpr uint32_t PT_NOTE = 4;       // Notes
constexpr uint32_t PT_PHDR = 6;       // Program header table
constexpr uint32_t PT_TLS = 7;        // Thread-local storage

// PS4-specific program header types
constexpr uint32_t PT_SCE_RELRO = 0x61000010;
constexpr uint32_t PT_SCE_DYNLIBDATA = 0x61000000;
constexpr uint32_t PT_SCE_PROCPARAM = 0x61000001;
constexpr uint32_t PT_SCE_MODULEPARAM = 0x61000002;

// Segment flags
constexpr uint32_t PF_X = 0x1;  // Execute
constexpr uint32_t PF_W = 0x2;  // Write
constexpr uint32_t PF_R = 0x4;  // Read

#pragma pack(push, 1)

/**
 * @brief ELF64 File Header
 */
struct Ehdr {
    uint8_t  e_ident[16];    // ELF identification
    uint16_t e_type;         // Object file type
    uint16_t e_machine;      // Machine type
    uint32_t e_version;      // Object file version
    uint64_t e_entry;        // Entry point address
    uint64_t e_phoff;        // Program header offset
    uint64_t e_shoff;        // Section header offset
    uint32_t e_flags;        // Processor-specific flags
    uint16_t e_ehsize;       // ELF header size
    uint16_t e_phentsize;    // Program header entry size
    uint16_t e_phnum;        // Number of program headers
    uint16_t e_shentsize;    // Section header entry size
    uint16_t e_shnum;        // Number of section headers
    uint16_t e_shstrndx;     // Section name string table index
};

/**
 * @brief ELF64 Program Header
 */
struct Phdr {
    uint32_t p_type;         // Segment type
    uint32_t p_flags;        // Segment flags
    uint64_t p_offset;       // Offset in file
    uint64_t p_vaddr;        // Virtual address in memory
    uint64_t p_paddr;        // Physical address (unused)
    uint64_t p_filesz;       // Size in file
    uint64_t p_memsz;        // Size in memory
    uint64_t p_align;        // Alignment
};

#pragma pack(pop)

} // namespace Elf64

// =============================================================================
// LOADED SEGMENT INFO
// =============================================================================

/**
 * @brief Information about a loaded segment
 */
struct LoadedSegment {
    uint64_t virtualAddress;
    uint64_t memorySize;
    uint64_t fileSize;
    uint32_t flags;  // PF_R, PF_W, PF_X
    std::string description;
};

/**
 * @brief Result of loading an ELF
 */
struct ElfLoadResult {
    uint64_t entryPoint = 0;
    uint64_t baseAddress = 0;
    uint64_t topAddress = 0;
    std::vector<LoadedSegment> segments;
    std::string elfType;
    bool isValid = false;
};

// =============================================================================
// WEAR_ELFLOADER CLASS
// =============================================================================

/**
 * @brief Loads PS4 ELF executables into memory
 */
class WeaR_ElfLoader {
public:
    using ErrorType = std::string;

    WeaR_ElfLoader() = default;

    /**
     * @brief Load an ELF file into memory
     * @param filepath Path to the ELF file
     * @param memory Reference to emulator memory
     * @return Load result with entry point, or error
     */
    [[nodiscard]] std::expected<ElfLoadResult, ErrorType> loadElf(
        const std::filesystem::path& filepath,
        WeaR_Memory& memory);

    /**
     * @brief Validate ELF header without loading
     * @param filepath Path to the ELF file
     * @return true if valid PS4 ELF
     */
    [[nodiscard]] std::expected<bool, ErrorType> validateElf(
        const std::filesystem::path& filepath);

    /**
     * @brief Get human-readable segment type name
     */
    [[nodiscard]] static std::string getSegmentTypeName(uint32_t type);

    /**
     * @brief Get segment flags as string (rwx)
     */
    [[nodiscard]] static std::string getSegmentFlagsString(uint32_t flags);

private:
    bool validateHeader(const Elf64::Ehdr& header) const;
    void loadSegment(const Elf64::Phdr& phdr, const std::vector<uint8_t>& fileData,
                     WeaR_Memory& memory, LoadedSegment& segment);
};

} // namespace WeaR
