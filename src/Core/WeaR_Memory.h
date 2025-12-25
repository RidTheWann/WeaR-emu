#pragma once

/**
 * @file WeaR_Memory.h
 * @brief PS4 Unified Memory Model Simulation
 * 
 * Simulates PS4's 8GB GDDR5 unified memory with proper region mapping.
 * Uses VirtualAlloc on Windows for large contiguous allocation.
 */

#include <cstdint>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <format>

namespace WeaR {

// =============================================================================
// PS4 MEMORY REGIONS (Virtual Address Layout)
// =============================================================================

namespace PS4Memory {
    // Total memory size (8 GB)
    constexpr uint64_t MEMORY_SIZE = 8ULL * 1024 * 1024 * 1024;  // 8 GB
    
    // Memory alignment for SIMD operations
    constexpr size_t ALIGNMENT = 16;  // 16-byte for SSE/AVX

    // PS4 Memory Map (approximate layout)
    namespace Region {
        // Kernel space (not directly accessible)
        constexpr uint64_t KERNEL_BASE     = 0xFFFF800000000000ULL;
        constexpr uint64_t KERNEL_SIZE     = 0x0000080000000000ULL;
        
        // User space executable
        constexpr uint64_t USER_BASE       = 0x0000000000400000ULL;  // 4 MB
        constexpr uint64_t USER_SIZE       = 0x0000007FFFE00000ULL;  // ~512 GB virtual
        
        // Heap region
        constexpr uint64_t HEAP_BASE       = 0x0000000200000000ULL;  // 8 GB mark
        constexpr uint64_t HEAP_SIZE       = 0x0000000400000000ULL;  // 16 GB virtual
        
        // Stack (grows down)
        constexpr uint64_t STACK_TOP       = 0x00007FFFFFFFF000ULL;
        constexpr uint64_t STACK_SIZE      = 0x0000000000800000ULL;  // 8 MB per thread
        
        // GPU accessible memory (VRAM mapped)
        constexpr uint64_t VRAM_BASE       = 0x0000000800000000ULL;  // 32 GB mark
        constexpr uint64_t VRAM_SIZE       = 0x0000000200000000ULL;  // 8 GB
        
        // Shared memory region
        constexpr uint64_t SHARED_BASE     = 0x0000001000000000ULL;  // 64 GB mark
        constexpr uint64_t SHARED_SIZE     = 0x0000000100000000ULL;  // 4 GB
    }

    // For our emulator: we map virtual addresses to physical 8GB block
    // Using simple offset: physical = virtual - USER_BASE (with wrapping)
    constexpr uint64_t PHYSICAL_MASK = MEMORY_SIZE - 1;  // 0x1FFFFFFFF (8GB mask)
}

// =============================================================================
// MEMORY ACCESS EXCEPTION
// =============================================================================

class MemoryAccessException : public std::runtime_error {
public:
    enum class Type {
        OutOfBounds,
        MisalignedAccess,
        WriteProtected,
        InvalidAddress
    };

    MemoryAccessException(Type type, uint64_t address, size_t size)
        : std::runtime_error(formatMessage(type, address, size))
        , m_type(type)
        , m_address(address)
        , m_size(size)
    {}

    [[nodiscard]] Type type() const { return m_type; }
    [[nodiscard]] uint64_t address() const { return m_address; }
    [[nodiscard]] size_t size() const { return m_size; }

private:
    static std::string formatMessage(Type type, uint64_t addr, size_t sz) {
        const char* typeStr = "Unknown";
        switch (type) {
            case Type::OutOfBounds: typeStr = "Segmentation Fault"; break;
            case Type::MisalignedAccess: typeStr = "Misaligned Access"; break;
            case Type::WriteProtected: typeStr = "Write Protected"; break;
            case Type::InvalidAddress: typeStr = "Invalid Address"; break;
        }
        return std::format("Memory Error: {} at 0x{:016X} (size: {})", typeStr, addr, sz);
    }

    Type m_type;
    uint64_t m_address;
    size_t m_size;
};

// =============================================================================
// WEAR_MEMORY CLASS
// =============================================================================

/**
 * @brief PS4 unified memory simulation
 * 
 * Allocates 8GB of contiguous memory using platform-specific APIs
 * for optimal performance with large allocations.
 */
class WeaR_Memory {
public:
    WeaR_Memory();
    ~WeaR_Memory();

    // Non-copyable
    WeaR_Memory(const WeaR_Memory&) = delete;
    WeaR_Memory& operator=(const WeaR_Memory&) = delete;

    // Move semantics
    WeaR_Memory(WeaR_Memory&& other) noexcept;
    WeaR_Memory& operator=(WeaR_Memory&& other) noexcept;

    /**
     * @brief Check if memory is allocated
     */
    [[nodiscard]] bool isInitialized() const { return m_memory != nullptr; }

    /**
     * @brief Get total memory size
     */
    [[nodiscard]] uint64_t size() const { return PS4Memory::MEMORY_SIZE; }

    /**
     * @brief Get raw pointer to base (use carefully)
     */
    [[nodiscard]] uint8_t* base() { return m_memory; }
    [[nodiscard]] const uint8_t* base() const { return m_memory; }

    // =========================================================================
    // SAFE MEMORY ACCESS (bounds checked)
    // =========================================================================

    /**
     * @brief Read value from virtual address
     * @tparam T Type to read (uint8_t, uint16_t, uint32_t, uint64_t, float, etc.)
     * @param virtualAddress PS4 virtual address
     * @return Value at address
     * @throws MemoryAccessException on invalid access
     */
    template<typename T>
    [[nodiscard]] T read(uint64_t virtualAddress) const {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
        
        uint64_t physicalAddr = translateAddress(virtualAddress);
        validateAccess(physicalAddr, sizeof(T));
        
        return *reinterpret_cast<const T*>(m_memory + physicalAddr);
    }

    /**
     * @brief Write value to virtual address
     * @tparam T Type to write
     * @param virtualAddress PS4 virtual address
     * @param value Value to write
     * @throws MemoryAccessException on invalid access
     */
    template<typename T>
    void write(uint64_t virtualAddress, T value) {
        static_assert(std::is_trivially_copyable_v<T>, "Type must be trivially copyable");
        
        uint64_t physicalAddr = translateAddress(virtualAddress);
        validateAccess(physicalAddr, sizeof(T));
        
        *reinterpret_cast<T*>(m_memory + physicalAddr) = value;
    }

    /**
     * @brief Read block of memory
     * @param virtualAddress Source address
     * @param dest Destination buffer
     * @param size Bytes to read
     */
    void readBlock(uint64_t virtualAddress, void* dest, size_t size) const;

    /**
     * @brief Write block of memory
     * @param virtualAddress Destination address
     * @param src Source buffer
     * @param size Bytes to write
     */
    void writeBlock(uint64_t virtualAddress, const void* src, size_t size);

    /**
     * @brief Fill memory region with value
     * @param virtualAddress Start address
     * @param value Byte value to fill
     * @param size Bytes to fill
     */
    void fill(uint64_t virtualAddress, uint8_t value, size_t size);

    /**
     * @brief Zero memory region
     */
    void zero(uint64_t virtualAddress, size_t size) {
        fill(virtualAddress, 0, size);
    }

    // =========================================================================
    // DIRECT ACCESS (no translation, faster but dangerous)
    // =========================================================================

    /**
     * @brief Get pointer to physical offset (bypasses virtual address translation)
     * @param physicalOffset Offset into 8GB block
     * @return Pointer to memory
     */
    [[nodiscard]] uint8_t* getPhysicalPointer(uint64_t physicalOffset);
    [[nodiscard]] const uint8_t* getPhysicalPointer(uint64_t physicalOffset) const;

    // =========================================================================
    // ADDRESS TRANSLATION
    // =========================================================================

    /**
     * @brief Translate PS4 virtual address to physical offset
     * @param virtualAddress PS4 virtual address
     * @return Physical offset in our 8GB block
     */
    [[nodiscard]] uint64_t translateAddress(uint64_t virtualAddress) const;

    /**
     * @brief Check if address is in valid range
     */
    [[nodiscard]] bool isValidAddress(uint64_t virtualAddress, size_t size = 1) const;

private:
    void validateAccess(uint64_t physicalAddr, size_t size) const;
    void allocateMemory();
    void freeMemory();

    uint8_t* m_memory = nullptr;
    bool m_ownsMemory = false;
};

} // namespace WeaR
