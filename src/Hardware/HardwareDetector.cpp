#include "HardwareDetector.h"

#include <algorithm>
#include <format>
#include <iostream>
#include <cstring>

namespace WeaR {

// =============================================================================
// WeaR_Specs Implementation
// =============================================================================

std::string WeaR_Specs::vramString() const {
    float gb = vramGB();
    if (gb >= 1.0f) {
        return std::format("{:.1f} GB", gb);
    } else {
        return std::format("{} MB", vramBytes / (1024 * 1024));
    }
}

std::string WeaR_Specs::tierString() const {
    return HardwareDetector::tierToString(tier);
}

// =============================================================================
// Static Utility Functions
// =============================================================================

std::string HardwareDetector::tierToString(GPUTier tier) {
    switch (tier) {
        case GPUTier::LowEnd:     return "Low-End";
        case GPUTier::MidRange:   return "Mid-Range";
        case GPUTier::HighEnd:    return "High-End";
        case GPUTier::Enthusiast: return "Enthusiast";
        default:                  return "Unknown";
    }
}

std::string HardwareDetector::getStatusColor(FrameGenStatus status) {
    switch (status) {
        case FrameGenStatus::Active:      return "#00ff9d";  // Neon Green
        case FrameGenStatus::Disabled:    return "#ff4444";  // Red
        case FrameGenStatus::Unsupported: return "#888888";  // Grey
        default:                          return "#666666";
    }
}

// =============================================================================
// Internal Implementation Class
// =============================================================================

class HardwareDetector::Impl {
public:
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice selectedDevice = VK_NULL_HANDLE;
    bool volkInitialized = false;

    ~Impl() {
        cleanup();
    }

    void cleanup() {
        if (instance != VK_NULL_HANDLE) {
            vkDestroyInstance(instance, nullptr);
            instance = VK_NULL_HANDLE;
        }
    }

    std::expected<void, std::string> initializeVolk() {
        if (volkInitialized) return {};

        VkResult result = volkInitialize();
        if (result != VK_SUCCESS) {
            return std::unexpected(std::format(
                "Failed to initialize Volk/Vulkan loader (VkResult: {}). "
                "Ensure Vulkan drivers are installed.", static_cast<int>(result)));
        }

        volkInitialized = true;
        return {};
    }

    std::expected<void, std::string> createInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "WeaR-emu Hardware Probe";
        appInfo.applicationVersion = VK_MAKE_API_VERSION(0, 0, 1, 0);
        appInfo.pEngineName = "WeaR Detector";
        appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Minimal extensions for probing
        std::vector<const char*> extensions = {
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
        };

        // Check extension availability
        uint32_t availableExtCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &availableExtCount, nullptr);
        std::vector<VkExtensionProperties> availableExts(availableExtCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &availableExtCount, availableExts.data());

        std::vector<const char*> enabledExtensions;
        for (const char* ext : extensions) {
            bool found = false;
            for (const auto& available : availableExts) {
                if (std::strcmp(ext, available.extensionName) == 0) {
                    found = true;
                    break;
                }
            }
            if (found) {
                enabledExtensions.push_back(ext);
            }
        }

        createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
        createInfo.ppEnabledExtensionNames = enabledExtensions.data();

        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
        if (result != VK_SUCCESS) {
            return std::unexpected(std::format(
                "Failed to create Vulkan instance (VkResult: {})", static_cast<int>(result)));
        }

        // Load instance-level functions
        volkLoadInstance(instance);
        return {};
    }

    std::expected<VkPhysicalDevice, std::string> selectBestDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            return std::unexpected("No Vulkan-capable GPU found. Please install graphics drivers.");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        // Score and select best device
        VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
        int32_t bestScore = -1;

        for (const auto& device : devices) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);

            int32_t score = 0;

            // Strongly prefer discrete GPUs
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
                score += 10000;
            } else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
                score += 1000;
            } else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU) {
                score += 500;
            }

            // Add score for capabilities
            score += static_cast<int32_t>(props.limits.maxImageDimension2D / 1000);
            score += static_cast<int32_t>(props.limits.maxComputeWorkGroupInvocations / 100);

            if (score > bestScore) {
                bestScore = score;
                bestDevice = device;
            }
        }

        if (bestDevice == VK_NULL_HANDLE) {
            return std::unexpected("No suitable GPU found");
        }

        selectedDevice = bestDevice;
        return bestDevice;
    }

    HardwareCapabilities queryCapabilities(VkPhysicalDevice device) {
        HardwareCapabilities caps{};

        // =====================================================================
        // Basic Properties
        // =====================================================================
        VkPhysicalDeviceProperties2 props2{};
        props2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;

        VkPhysicalDeviceSubgroupProperties subgroupProps{};
        subgroupProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;
        props2.pNext = &subgroupProps;

        vkGetPhysicalDeviceProperties2(device, &props2);

        const auto& props = props2.properties;
        caps.gpuName = props.deviceName;
        caps.vendorID = props.vendorID;
        caps.deviceID = props.deviceID;
        caps.deviceType = props.deviceType;

        // Parse driver version
        uint32_t driverVer = props.driverVersion;
        if (caps.vendorID == 0x10DE) { // NVIDIA encoding
            caps.driverVersion = std::format("{}.{}.{}.{}",
                (driverVer >> 22) & 0x3FF,
                (driverVer >> 14) & 0xFF,
                (driverVer >> 6) & 0xFF,
                driverVer & 0x3F);
        } else if (caps.vendorID == 0x8086) { // Intel encoding
            caps.driverVersion = std::format("{}.{}",
                driverVer >> 14, driverVer & 0x3FFF);
        } else { // Standard Vulkan encoding
            caps.driverVersion = std::format("{}.{}.{}",
                VK_API_VERSION_MAJOR(driverVer),
                VK_API_VERSION_MINOR(driverVer),
                VK_API_VERSION_PATCH(driverVer));
        }

        // Compute limits
        caps.maxComputeWorkGroupSize[0] = props.limits.maxComputeWorkGroupSize[0];
        caps.maxComputeWorkGroupSize[1] = props.limits.maxComputeWorkGroupSize[1];
        caps.maxComputeWorkGroupSize[2] = props.limits.maxComputeWorkGroupSize[2];
        caps.maxComputeSharedMemory = props.limits.maxComputeSharedMemorySize;
        caps.maxComputeWorkGroupInvocations = props.limits.maxComputeWorkGroupInvocations;
        caps.subgroupSize = subgroupProps.subgroupSize;
        caps.supportsSubgroupOperations = 
            (subgroupProps.supportedOperations & VK_SUBGROUP_FEATURE_ARITHMETIC_BIT) != 0;

        // =====================================================================
        // Features (FP16, Int8)
        // =====================================================================
        VkPhysicalDeviceFeatures2 features2{};
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

        VkPhysicalDeviceFloat16Int8FeaturesKHR fp16Int8{};
        fp16Int8.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR;
        features2.pNext = &fp16Int8;

        VkPhysicalDevice16BitStorageFeatures storage16{};
        storage16.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
        fp16Int8.pNext = &storage16;

        vkGetPhysicalDeviceFeatures2(device, &features2);

        caps.supportsFloat16 = (fp16Int8.shaderFloat16 == VK_TRUE);
        caps.supportsInt8 = (fp16Int8.shaderInt8 == VK_TRUE);
        caps.supportsShaderFloat16Int8 = caps.supportsFloat16 || caps.supportsInt8;
        caps.supportsStorageBuffer16Bit = (storage16.storageBuffer16BitAccess == VK_TRUE);

        // =====================================================================
        // Memory (VRAM)
        // =====================================================================
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(device, &memProps);

        for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i) {
            const auto& heap = memProps.memoryHeaps[i];
            if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
                caps.vramBytes += heap.size;
            } else {
                caps.sharedSystemMemory += heap.size;
            }
        }

        // =====================================================================
        // Performance Estimation
        // =====================================================================
        estimatePerformance(caps);

        // =====================================================================
        // WeaR-Gen Capability Decision
        // =====================================================================
        determineFrameGenCapability(caps);

        return caps;
    }

    void estimatePerformance(HardwareCapabilities& caps) {
        // Heuristic-based TFLOP estimation
        // Based on: work group size, shared memory, subgroup size, VRAM, device type
        
        float baseMultiplier = 1.0f;

        // Vendor adjustments
        switch (caps.vendorID) {
            case 0x10DE: baseMultiplier = 1.3f; break;  // NVIDIA
            case 0x1002: baseMultiplier = 1.2f; break;  // AMD
            case 0x8086: baseMultiplier = 0.6f; break;  // Intel (usually integrated)
            default:     baseMultiplier = 0.8f;
        }

        // Compute factor from work group invocations
        // Modern GPUs typically have 1024-1536, older have 512-768
        float computeFactor = static_cast<float>(caps.maxComputeWorkGroupInvocations) / 1024.0f;

        // Shared memory factor (48KB is modern, 32KB is older)
        float sharedMemFactor = static_cast<float>(caps.maxComputeSharedMemory) / 49152.0f;

        // Subgroup (warp/wavefront) size factor
        float subgroupFactor = static_cast<float>(caps.subgroupSize) / 32.0f;

        // VRAM factor (normalized around 8GB)
        float vramGB = caps.vramGB();
        float vramFactor = std::clamp(vramGB / 8.0f, 0.25f, 3.0f);

        // Device type factor
        float typeFactor = 1.0f;
        switch (caps.deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                typeFactor = 2.0f;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                typeFactor = 0.4f;
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                typeFactor = 1.0f;
                break;
            default:
                typeFactor = 0.5f;
        }

        // FP16 bonus (modern architectures)
        float fp16Bonus = caps.supportsFloat16 ? 1.5f : 1.0f;

        // Combined estimation
        caps.estimatedTFLOPs = baseMultiplier * computeFactor * sharedMemFactor * 
                               subgroupFactor * vramFactor * typeFactor * fp16Bonus;

        // Clamp to reasonable range
        caps.estimatedTFLOPs = std::clamp(caps.estimatedTFLOPs, 0.3f, 150.0f);

        // Classify tier
        if (caps.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU || 
            caps.estimatedTFLOPs < 2.0f) {
            caps.tier = GPUTier::LowEnd;
        } else if (caps.estimatedTFLOPs < 6.0f) {
            caps.tier = GPUTier::MidRange;
        } else if (caps.estimatedTFLOPs < 20.0f) {
            caps.tier = GPUTier::HighEnd;
        } else {
            caps.tier = GPUTier::Enthusiast;
        }
    }

    void determineFrameGenCapability(HardwareCapabilities& caps) {
        caps.canRunFrameGen = true;
        caps.frameGenDisableReason.clear();

        // ===== CHECK 1: TFLOP Threshold =====
        if (caps.estimatedTFLOPs < TFLOP_THRESHOLD) {
            caps.canRunFrameGen = false;
            caps.frameGenDisableReason = std::format(
                "Insufficient compute power: {:.1f} TFLOPs (requires >= {:.1f})",
                caps.estimatedTFLOPs, TFLOP_THRESHOLD);
            return;
        }

        // ===== CHECK 2: VRAM Threshold (2GB minimum) =====
        if (caps.vramBytes < VRAM_THRESHOLD_BYTES) {
            caps.canRunFrameGen = false;
            caps.frameGenDisableReason = std::format(
                "Insufficient VRAM: {} (requires >= 2 GB)",
                caps.vramString());
            return;
        }

        // ===== CHECK 3: Integrated GPU Warning =====
        if (caps.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            caps.canRunFrameGen = false;
            caps.frameGenDisableReason = "Integrated GPU detected - WeaR-Gen disabled for stability";
            return;
        }

        // ===== CHECK 4: FP16 Preferred (soft requirement) =====
        if (!caps.supportsFloat16) {
            // Still allow but with reduced quality
            // caps.canRunFrameGen stays true
            // Log warning but don't disable
        }
    }
};

// =============================================================================
// HardwareDetector Public Methods
// =============================================================================

HardwareDetector::~HardwareDetector() = default;

HardwareDetector::HardwareDetector(HardwareDetector&&) noexcept = default;
HardwareDetector& HardwareDetector::operator=(HardwareDetector&&) noexcept = default;

std::expected<WeaR_Specs, HardwareDetector::ErrorType> HardwareDetector::detectCapabilities() {
    auto result = detectDetailedCapabilities();
    if (!result) {
        return std::unexpected(result.error());
    }
    
    // Convert to simpler WeaR_Specs
    const HardwareCapabilities& caps = result.value();
    WeaR_Specs specs;
    specs.gpuName = caps.gpuName;
    specs.driverVersion = caps.driverVersion;
    specs.vendorID = caps.vendorID;
    specs.estimatedTFLOPs = caps.estimatedTFLOPs;
    specs.vramBytes = caps.vramBytes;
    specs.tier = caps.tier;
    specs.supportsFloat16 = caps.supportsFloat16;
    specs.supportsShaderFloat16Int8 = caps.supportsShaderFloat16Int8;
    specs.canRunFrameGen = caps.canRunFrameGen;
    specs.frameGenDisableReason = caps.frameGenDisableReason;
    
    return specs;
}

std::expected<HardwareCapabilities, HardwareDetector::ErrorType> HardwareDetector::detectDetailedCapabilities() {
    Impl impl;

    // Initialize volk (Vulkan loader)
    if (auto result = impl.initializeVolk(); !result) {
        return std::unexpected(result.error());
    }

    // Create temporary Vulkan instance for probing
    if (auto result = impl.createInstance(); !result) {
        return std::unexpected(result.error());
    }

    // Select best GPU
    auto deviceResult = impl.selectBestDevice();
    if (!deviceResult) {
        return std::unexpected(deviceResult.error());
    }

    // Query all capabilities
    HardwareCapabilities caps = impl.queryCapabilities(deviceResult.value());

    // Cleanup happens automatically in Impl destructor
    return caps;
}

} // namespace WeaR
