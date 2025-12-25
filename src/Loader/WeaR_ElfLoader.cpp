#include "WeaR_ElfLoader.h"

#include <fstream>
#include <iostream>
#include <format>
#include <cstring>

// Qt logging
#include <QDebug>

namespace WeaR {

// =============================================================================
// SEGMENT TYPE NAMES
// =============================================================================

std::string WeaR_ElfLoader::getSegmentTypeName(uint32_t type) {
    switch (type) {
        case Elf64::PT_NULL:    return "NULL";
        case Elf64::PT_LOAD:    return "LOAD";
        case Elf64::PT_DYNAMIC: return "DYNAMIC";
        case Elf64::PT_INTERP:  return "INTERP";
        case Elf64::PT_NOTE:    return "NOTE";
        case Elf64::PT_PHDR:    return "PHDR";
        case Elf64::PT_TLS:     return "TLS";
        case Elf64::PT_SCE_RELRO:       return "SCE_RELRO";
        case Elf64::PT_SCE_DYNLIBDATA:  return "SCE_DYNLIBDATA";
        case Elf64::PT_SCE_PROCPARAM:   return "SCE_PROCPARAM";
        case Elf64::PT_SCE_MODULEPARAM: return "SCE_MODULEPARAM";
        default: return std::format("UNKNOWN(0x{:08X})", type);
    }
}

std::string WeaR_ElfLoader::getSegmentFlagsString(uint32_t flags) {
    std::string result;
    result += (flags & Elf64::PF_R) ? 'r' : '-';
    result += (flags & Elf64::PF_W) ? 'w' : '-';
    result += (flags & Elf64::PF_X) ? 'x' : '-';
    return result;
}

// =============================================================================
// HEADER VALIDATION
// =============================================================================

bool WeaR_ElfLoader::validateHeader(const Elf64::Ehdr& header) const {
    // Check magic bytes
    if (std::memcmp(header.e_ident, Elf64::MAGIC, 4) != 0) {
        qWarning() << "[ElfLoader] Invalid ELF magic";
        return false;
    }

    // Check 64-bit class
    if (header.e_ident[4] != Elf64::CLASS_64) {
        qWarning() << "[ElfLoader] Not a 64-bit ELF (PS4 requires ELF64)";
        return false;
    }

    // Check little-endian
    if (header.e_ident[5] != Elf64::DATA_LSB) {
        qWarning() << "[ElfLoader] Not little-endian";
        return false;
    }

    // Check machine type (x86-64)
    if (header.e_machine != Elf64::EM_X86_64) {
        qWarning() << "[ElfLoader] Not x86-64 architecture";
        return false;
    }

    // Check type (executable or shared object)
    if (header.e_type != Elf64::ET_EXEC && header.e_type != Elf64::ET_DYN) {
        qWarning() << "[ElfLoader] Not an executable or shared object";
        return false;
    }

    // OS ABI check (FreeBSD/PS4) - warning only, don't reject
    uint8_t osabi = header.e_ident[7];
    if (osabi != Elf64::OSABI_FREEBSD && osabi != 0) {
        qWarning() << "[ElfLoader] Warning: OS ABI is" << osabi << "(expected FreeBSD/9)";
    }

    return true;
}

// =============================================================================
// VALIDATE WITHOUT LOADING
// =============================================================================

std::expected<bool, WeaR_ElfLoader::ErrorType> WeaR_ElfLoader::validateElf(
    const std::filesystem::path& filepath)
{
    if (!std::filesystem::exists(filepath)) {
        return std::unexpected(std::format("File not found: {}", filepath.string()));
    }

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return std::unexpected(std::format("Cannot open file: {}", filepath.string()));
    }

    Elf64::Ehdr header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    
    if (file.gcount() != sizeof(header)) {
        return std::unexpected("File too small to contain ELF header");
    }

    return validateHeader(header);
}

// =============================================================================
// LOAD SEGMENT
// =============================================================================

void WeaR_ElfLoader::loadSegment(const Elf64::Phdr& phdr, 
                                  const std::vector<uint8_t>& fileData,
                                  WeaR_Memory& memory, 
                                  LoadedSegment& segment)
{
    segment.virtualAddress = phdr.p_vaddr;
    segment.memorySize = phdr.p_memsz;
    segment.fileSize = phdr.p_filesz;
    segment.flags = phdr.p_flags;
    segment.description = std::format("{} {} at 0x{:016X}", 
        getSegmentTypeName(phdr.p_type),
        getSegmentFlagsString(phdr.p_flags),
        phdr.p_vaddr);

    // Only load if there's file data
    if (phdr.p_filesz > 0 && phdr.p_offset + phdr.p_filesz <= fileData.size()) {
        memory.writeBlock(phdr.p_vaddr, fileData.data() + phdr.p_offset, phdr.p_filesz);
    }

    // Zero-fill BSS (memory size > file size)
    if (phdr.p_memsz > phdr.p_filesz) {
        uint64_t bssStart = phdr.p_vaddr + phdr.p_filesz;
        uint64_t bssSize = phdr.p_memsz - phdr.p_filesz;
        memory.zero(bssStart, bssSize);
    }
}

// =============================================================================
// MAIN LOAD FUNCTION
// =============================================================================

std::expected<ElfLoadResult, WeaR_ElfLoader::ErrorType> WeaR_ElfLoader::loadElf(
    const std::filesystem::path& filepath,
    WeaR_Memory& memory)
{
    ElfLoadResult result;

    qDebug() << "[ElfLoader] Loading:" << QString::fromStdString(filepath.string());

    // Validate file exists
    if (!std::filesystem::exists(filepath)) {
        return std::unexpected(std::format("File not found: {}", filepath.string()));
    }

    // Read entire file
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return std::unexpected(std::format("Cannot open file: {}", filepath.string()));
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0);

    qDebug() << "[ElfLoader] File size:" << fileSize << "bytes";

    if (fileSize < sizeof(Elf64::Ehdr)) {
        return std::unexpected("File too small to contain ELF header");
    }

    std::vector<uint8_t> fileData(fileSize);
    file.read(reinterpret_cast<char*>(fileData.data()), fileSize);
    file.close();

    // Parse ELF header
    const Elf64::Ehdr* header = reinterpret_cast<const Elf64::Ehdr*>(fileData.data());

    if (!validateHeader(*header)) {
        return std::unexpected("Invalid ELF header (not a valid PS4 executable)");
    }

    result.entryPoint = header->e_entry;
    result.elfType = (header->e_type == Elf64::ET_EXEC) ? "Executable" : "Shared Object";

    qDebug() << "[ElfLoader] Type:" << QString::fromStdString(result.elfType);
    qDebug() << "[ElfLoader] Entry Point: 0x" << QString::number(result.entryPoint, 16).toUpper();
    qDebug() << "[ElfLoader] Program Headers:" << header->e_phnum;

    // Validate program header table
    if (header->e_phoff + header->e_phnum * sizeof(Elf64::Phdr) > fileSize) {
        return std::unexpected("Invalid program header table offset");
    }

    const Elf64::Phdr* phdrs = reinterpret_cast<const Elf64::Phdr*>(
        fileData.data() + header->e_phoff);

    // Process each program header
    uint64_t lowestAddr = UINT64_MAX;
    uint64_t highestAddr = 0;

    for (uint16_t i = 0; i < header->e_phnum; ++i) {
        const Elf64::Phdr& phdr = phdrs[i];

        qDebug() << "[ElfLoader] Segment" << i << ":"
                 << QString::fromStdString(getSegmentTypeName(phdr.p_type))
                 << QString::fromStdString(getSegmentFlagsString(phdr.p_flags))
                 << "vaddr=0x" << QString::number(phdr.p_vaddr, 16)
                 << "memsz=" << phdr.p_memsz;

        // Only load PT_LOAD segments
        if (phdr.p_type == Elf64::PT_LOAD) {
            // Validate segment doesn't exceed file
            if (phdr.p_offset + phdr.p_filesz > fileSize) {
                qWarning() << "[ElfLoader] Segment" << i << "extends beyond file";
                continue;
            }

            // Check memory bounds
            if (!memory.isValidAddress(phdr.p_vaddr, phdr.p_memsz)) {
                qWarning() << "[ElfLoader] Segment" << i << "exceeds memory bounds, skipping";
                continue;
            }

            LoadedSegment segment;
            loadSegment(phdr, fileData, memory, segment);
            result.segments.push_back(segment);

            // Track address range
            if (phdr.p_vaddr < lowestAddr) {
                lowestAddr = phdr.p_vaddr;
            }
            if (phdr.p_vaddr + phdr.p_memsz > highestAddr) {
                highestAddr = phdr.p_vaddr + phdr.p_memsz;
            }

            qDebug() << "[ElfLoader]   Loaded" << phdr.p_filesz << "bytes to 0x"
                     << QString::number(phdr.p_vaddr, 16).toUpper();
        }
    }

    result.baseAddress = lowestAddr;
    result.topAddress = highestAddr;
    result.isValid = !result.segments.empty();

    if (result.isValid) {
        qDebug() << "[ElfLoader] Successfully loaded" << result.segments.size() << "segments";
        qDebug() << "[ElfLoader] Base: 0x" << QString::number(result.baseAddress, 16).toUpper();
        qDebug() << "[ElfLoader] Top:  0x" << QString::number(result.topAddress, 16).toUpper();
        qDebug() << "[ElfLoader] Entry: 0x" << QString::number(result.entryPoint, 16).toUpper();
    } else {
        return std::unexpected("No loadable segments found in ELF");
    }

    return result;
}

// =============================================================================
// LOAD FROM MEMORY BUFFER (for PKG extraction)
// =============================================================================

std::expected<ElfLoadResult, WeaR_ElfLoader::ErrorType> WeaR_ElfLoader::loadElfFromMemory(
    const std::vector<uint8_t>& data,
    WeaR_Memory& memory)
{
    ElfLoadResult result;
    size_t fileSize = data.size();

    qDebug() << "[ElfLoader] Loading ELF from memory buffer (" << fileSize << "bytes)";

    if (fileSize < sizeof(Elf64::Ehdr)) {
        return std::unexpected("Buffer too small to contain ELF header");
    }

    // Parse ELF header
    const Elf64::Ehdr* header = reinterpret_cast<const Elf64::Ehdr*>(data.data());

    if (!validateHeader(*header)) {
        return std::unexpected("Invalid ELF header (not a valid PS4 executable)");
    }

    result.entryPoint = header->e_entry;
    result.elfType = (header->e_type == Elf64::ET_EXEC) ? "Executable" : "Shared Object";

    qDebug() << "[ElfLoader] Type:" << QString::fromStdString(result.elfType);
    qDebug() << "[ElfLoader] Entry Point: 0x" << QString::number(result.entryPoint, 16).toUpper();
    qDebug() << "[ElfLoader] Program Headers:" << header->e_phnum;

    // Validate program header table
    if (header->e_phoff + header->e_phnum * sizeof(Elf64::Phdr) > fileSize) {
        return std::unexpected("Invalid program header table offset");
    }

    const Elf64::Phdr* phdrs = reinterpret_cast<const Elf64::Phdr*>(
        data.data() + header->e_phoff);

    // Process each program header
    uint64_t lowestAddr = UINT64_MAX;
    uint64_t highestAddr = 0;

    for (uint16_t i = 0; i < header->e_phnum; ++i) {
        const Elf64::Phdr& phdr = phdrs[i];

        // Only load PT_LOAD segments
        if (phdr.p_type == Elf64::PT_LOAD) {
            if (phdr.p_offset + phdr.p_filesz > fileSize) {
                qWarning() << "[ElfLoader] Segment" << i << "extends beyond buffer";
                continue;
            }

            if (!memory.isValidAddress(phdr.p_vaddr, phdr.p_memsz)) {
                qWarning() << "[ElfLoader] Segment" << i << "exceeds memory bounds, skipping";
                continue;
            }

            LoadedSegment segment;
            loadSegment(phdr, data, memory, segment);
            result.segments.push_back(segment);

            if (phdr.p_vaddr < lowestAddr) lowestAddr = phdr.p_vaddr;
            if (phdr.p_vaddr + phdr.p_memsz > highestAddr) highestAddr = phdr.p_vaddr + phdr.p_memsz;

            qDebug() << "[ElfLoader]   Loaded" << phdr.p_filesz << "bytes to 0x"
                     << QString::number(phdr.p_vaddr, 16).toUpper();
        }
    }

    result.baseAddress = lowestAddr;
    result.topAddress = highestAddr;
    result.isValid = !result.segments.empty();

    if (result.isValid) {
        qDebug() << "[ElfLoader] Successfully loaded" << result.segments.size() << "segments from memory";
    } else {
        return std::unexpected("No loadable segments found in ELF buffer");
    }

    return result;
}

} // namespace WeaR
