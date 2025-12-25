#include "WeaR_Memory.h"

#include <cstring>
#include <iostream>
#include <format>

#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace WeaR {

// =============================================================================
// CONSTRUCTOR / DESTRUCTOR
// =============================================================================

WeaR_Memory::WeaR_Memory() {
    allocateMemory();
}

WeaR_Memory::~WeaR_Memory() {
    freeMemory();
}

WeaR_Memory::WeaR_Memory(WeaR_Memory&& other) noexcept
    : m_memory(other.m_memory)
    , m_ownsMemory(other.m_ownsMemory)
{
    other.m_memory = nullptr;
    other.m_ownsMemory = false;
}

WeaR_Memory& WeaR_Memory::operator=(WeaR_Memory&& other) noexcept {
    if (this != &other) {
        freeMemory();
        m_memory = other.m_memory;
        m_ownsMemory = other.m_ownsMemory;
        other.m_memory = nullptr;
        other.m_ownsMemory = false;
    }
    return *this;
}

// =============================================================================
// MEMORY ALLOCATION
// =============================================================================

void WeaR_Memory::allocateMemory() {
    if (m_memory) return;

    const size_t size = PS4Memory::MEMORY_SIZE;
    
    std::cout << std::format("[Memory] Allocating {} GB unified memory...\n", size / (1024 * 1024 * 1024));

#ifdef _WIN32
    // Windows: Use VirtualAlloc for large aligned allocation
    m_memory = static_cast<uint8_t*>(VirtualAlloc(
        nullptr,                        // Let system choose address
        size,                           // 8 GB
        MEM_COMMIT | MEM_RESERVE,       // Commit immediately
        PAGE_READWRITE                  // Read/Write access
    ));

    if (!m_memory) {
        DWORD error = GetLastError();
        std::cerr << std::format("[Memory] VirtualAlloc failed with error: {}\n", error);
        
        // Fallback: try smaller allocation for development
        const size_t fallbackSize = 512 * 1024 * 1024;  // 512 MB
        std::cout << "[Memory] Trying fallback allocation of 512 MB...\n";
        
        m_memory = static_cast<uint8_t*>(VirtualAlloc(
            nullptr, fallbackSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
        
        if (m_memory) {
            std::cout << "[Memory] Fallback allocation successful (limited mode)\n";
        }
    }
#else
    // Linux/macOS: Use mmap
    m_memory = static_cast<uint8_t*>(mmap(
        nullptr,
        size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE,
        -1, 0
    ));

    if (m_memory == MAP_FAILED) {
        m_memory = nullptr;
        std::cerr << "[Memory] mmap failed\n";
    }
#endif

    if (m_memory) {
        m_ownsMemory = true;
        std::cout << std::format("[Memory] Allocated at address: 0x{:016X}\n", 
                                  reinterpret_cast<uintptr_t>(m_memory));
    } else {
        throw std::runtime_error("Failed to allocate PS4 unified memory");
    }
}

void WeaR_Memory::freeMemory() {
    if (!m_memory || !m_ownsMemory) return;

    std::cout << "[Memory] Freeing unified memory...\n";

#ifdef _WIN32
    VirtualFree(m_memory, 0, MEM_RELEASE);
#else
    munmap(m_memory, PS4Memory::MEMORY_SIZE);
#endif

    m_memory = nullptr;
    m_ownsMemory = false;
}

// =============================================================================
// ADDRESS TRANSLATION
// =============================================================================

uint64_t WeaR_Memory::translateAddress(uint64_t virtualAddress) const {
    // Simple mapping: wrap virtual address into our 8GB physical block
    // This is a simplified model - real PS4 has complex page tables
    
    // For user-space addresses, subtract USER_BASE and mask to physical size
    if (virtualAddress >= PS4Memory::Region::USER_BASE) {
        uint64_t offset = virtualAddress - PS4Memory::Region::USER_BASE;
        return offset & PS4Memory::PHYSICAL_MASK;
    }
    
    // Direct mapping for low addresses (used in some contexts)
    return virtualAddress & PS4Memory::PHYSICAL_MASK;
}

bool WeaR_Memory::isValidAddress(uint64_t virtualAddress, size_t size) const {
    if (!m_memory) return false;
    
    uint64_t physicalAddr = translateAddress(virtualAddress);
    return (physicalAddr + size) <= PS4Memory::MEMORY_SIZE;
}

// =============================================================================
// MEMORY ACCESS
// =============================================================================

void WeaR_Memory::validateAccess(uint64_t physicalAddr, size_t size) const {
    if (!m_memory) {
        throw MemoryAccessException(
            MemoryAccessException::Type::InvalidAddress, physicalAddr, size);
    }
    
    if (physicalAddr + size > PS4Memory::MEMORY_SIZE) {
        throw MemoryAccessException(
            MemoryAccessException::Type::OutOfBounds, physicalAddr, size);
    }
}

void WeaR_Memory::readBlock(uint64_t virtualAddress, void* dest, size_t size) const {
    if (!dest || size == 0) return;
    
    uint64_t physicalAddr = translateAddress(virtualAddress);
    validateAccess(physicalAddr, size);
    
    std::memcpy(dest, m_memory + physicalAddr, size);
}

void WeaR_Memory::writeBlock(uint64_t virtualAddress, const void* src, size_t size) {
    if (!src || size == 0) return;
    
    uint64_t physicalAddr = translateAddress(virtualAddress);
    validateAccess(physicalAddr, size);
    
    std::memcpy(m_memory + physicalAddr, src, size);
}

void WeaR_Memory::fill(uint64_t virtualAddress, uint8_t value, size_t size) {
    if (size == 0) return;
    
    uint64_t physicalAddr = translateAddress(virtualAddress);
    validateAccess(physicalAddr, size);
    
    std::memset(m_memory + physicalAddr, value, size);
}

uint8_t* WeaR_Memory::getPhysicalPointer(uint64_t physicalOffset) {
    validateAccess(physicalOffset, 1);
    return m_memory + physicalOffset;
}

const uint8_t* WeaR_Memory::getPhysicalPointer(uint64_t physicalOffset) const {
    validateAccess(physicalOffset, 1);
    return m_memory + physicalOffset;
}

} // namespace WeaR
