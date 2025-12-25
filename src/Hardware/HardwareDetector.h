#pragma once

/**
 * @file HardwareDetector.h
 * @brief Hardware capability detection for adaptive WeaR-Gen feature enabling
 * 
 * Uses volk for dynamic Vulkan loading and creates a temporary instance
 * to query GPU capabilities before full engine initialization.
 */

// Volk must be included before Vulkan
#if defined(_WIN32)
    #define VK_USE_PLATFORM_WIN32_KHR
#endif
#ifndef VK_NO_PROTOTYPES
    #define VK_NO_PROTOTYPES
#endif
#include <volk.h>
#include <vk_mem_alloc.h>

#include <string>
#include <cstdint>
#include <optional>
#include <expected>
#include <vector>

namespace WeaR {

/**
 * @brief GPU capability tier for adaptive feature selection
 */
enum class GPUTier : uint8_t {
    Unknown = 0,
    LowEnd,      // Integrated/Old discrete - No WeaR-Gen
    MidRange,    // Entry discrete - Limited WeaR-Gen  
    HighEnd,     // Modern discrete - Full WeaR-Gen
    Enthusiast   // Flagship - All features + extras
};

/**
 * @brief WeaR-Gen capability status
 */
enum class FrameGenStatus : uint8_t {
    Unsupported,   // Hardware cannot run WeaR-Gen
    Disabled,      // Can run but user disabled
    Active         // Running WeaR-Gen
};

/**
 * @brief Simplified specs struct for quick capability checks
 * 
 * This is the primary interface for checking if WeaR-Gen can run.
 */
struct WeaR_Specs {
    // Device Info
    std::string gpuName = "Unknown GPU";
    std::string driverVersion = "Unknown";
    uint32_t vendorID = 0;
    
    // Key Metrics
    float estimatedTFLOPs = 0.0f;
    uint64_t vramBytes = 0;
    GPUTier tier = GPUTier::Unknown;
    
    // Feature Support
    bool supportsFloat16 = false;
    bool supportsShaderFloat16Int8 = false;
    
    // === CRITICAL DECISION ===
    bool canRunFrameGen = false;
    std::string frameGenDisableReason;
    
    // Convenience methods
    [[nodiscard]] float vramGB() const { 
        return static_cast<float>(vramBytes) / (1024.0f * 1024.0f * 1024.0f); 
    }
    
    [[nodiscard]] std::string vramString() const;
    [[nodiscard]] std::string tierString() const;
};

/**
 * @brief Detailed hardware capabilities (extended info)
 */
struct HardwareCapabilities : public WeaR_Specs {
    // Device Type
    VkPhysicalDeviceType deviceType = VK_PHYSICAL_DEVICE_TYPE_OTHER;
    uint32_t deviceID = 0;
    
    // Compute Limits
    uint32_t maxComputeWorkGroupSize[3] = {0, 0, 0};
    uint32_t maxComputeSharedMemory = 0;
    uint32_t maxComputeWorkGroupInvocations = 0;
    uint32_t subgroupSize = 0;
    
    // Extended Features
    bool supportsInt8 = false;
    bool supportsStorageBuffer16Bit = false;
    bool supportsSubgroupOperations = false;
    
    // Memory Details
    uint64_t sharedSystemMemory = 0;
};

/**
 * @brief Standalone hardware detector using volk
 * 
 * Creates a temporary Vulkan instance to query GPU capabilities
 * before the main render engine is initialized.
 */
class HardwareDetector {
public:
    using ErrorType = std::string;

    HardwareDetector() = default;
    ~HardwareDetector();

    // Non-copyable
    HardwareDetector(const HardwareDetector&) = delete;
    HardwareDetector& operator=(const HardwareDetector&) = delete;
    HardwareDetector(HardwareDetector&&) noexcept;
    HardwareDetector& operator=(HardwareDetector&&) noexcept;

    /**
     * @brief Detect hardware capabilities (main entry point)
     * 
     * Initializes volk, creates a temporary Vulkan instance, queries
     * the best available GPU, and returns capability info.
     * 
     * @return WeaR_Specs with canRunFrameGen decision, or error
     */
    [[nodiscard]] static std::expected<WeaR_Specs, ErrorType> detectCapabilities();

    /**
     * @brief Get full detailed capabilities
     * 
     * Same as detectCapabilities but returns extended info.
     */
    [[nodiscard]] static std::expected<HardwareCapabilities, ErrorType> detectDetailedCapabilities();

    /**
     * @brief Get human-readable tier string
     */
    [[nodiscard]] static std::string tierToString(GPUTier tier);

    /**
     * @brief Get status color for UI
     * @return Hex color string (#00ff9d for active, #888888 for unsupported, etc.)
     */
    [[nodiscard]] static std::string getStatusColor(FrameGenStatus status);

private:
    // Thresholds for WeaR-Gen capability
    static constexpr float TFLOP_THRESHOLD = 4.0f;
    static constexpr uint64_t VRAM_THRESHOLD_BYTES = 2ULL * 1024 * 1024 * 1024; // 2GB

    // Internal implementation
    class Impl;
};

} // namespace WeaR
