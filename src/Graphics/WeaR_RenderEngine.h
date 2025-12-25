#pragma once

/**
 * @file WeaR_RenderEngine.h
 * @brief Vulkan 1.3 Render Engine with adaptive WeaR-Gen compute pipeline
 * 
 * Features:
 * - Dynamic Rendering (VK_KHR_dynamic_rendering)
 * - VMA for memory management
 * - Adaptive pipeline: Compute (WeaR-Gen) or Passthrough based on hardware
 * - Proper synchronization between Compute and Graphics queues
 * - Visual test mode with spinning neon green triangle
 */

#if defined(_WIN32)
    #define VK_USE_PLATFORM_WIN32_KHR
#endif
#ifndef VK_NO_PROTOTYPES
    #define VK_NO_PROTOTYPES
#endif
#include <volk.h>

#include "Hardware/HardwareDetector.h"

#include <string>
#include <vector>
#include <array>
#include <optional>
#include <expected>
#include <functional>
#include <memory>
#include <chrono>

// Forward declare VMA
struct VmaAllocator_T;
typedef VmaAllocator_T* VmaAllocator;
struct VmaAllocation_T;
typedef VmaAllocation_T* VmaAllocation;

// Forward declare Windows handle
#ifdef _WIN32
struct HWND__;
typedef HWND__* HWND;
#endif

namespace WeaR {

// =============================================================================
// CONFIGURATION STRUCTURES
// =============================================================================

/**
 * @brief Vertex for test triangle
 */
struct Vertex {
    float position[3];
    float color[3];
};

/**
 * @brief Frame generation push constants (matches shader)
 */
struct FrameGenPushConstants {
    float interpolationFactor = 0.5f;
    float motionScale = 1.0f;
    float blendWeight = 0.3f;
    int   blockRadius = 4;
};

/**
 * @brief Graphics push constants for rotation
 */
struct GraphicsPushConstants {
    float rotationAngle = 0.0f;
    float padding[3] = {0, 0, 0};
};

/**
 * @brief Render engine configuration
 */
struct RenderEngineConfig {
    std::string appName = "WeaR-emu";
    uint32_t windowWidth = 1920;
    uint32_t windowHeight = 1080;
    bool enableValidation = true;
    bool vsyncEnabled = true;
};

/**
 * @brief Buffer with VMA allocation
 */
struct AllocatedBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    VkDeviceSize size = 0;
};

/**
 * @brief Image with VMA allocation
 */
struct AllocatedImage {
    VkImage image = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent2D extent = {0, 0};
};

/**
 * @brief Per-frame synchronization resources
 */
struct FrameSyncObjects {
    VkSemaphore imageAvailable = VK_NULL_HANDLE;
    VkSemaphore renderFinished = VK_NULL_HANDLE;
    VkSemaphore computeFinished = VK_NULL_HANDLE;
    VkFence inFlight = VK_NULL_HANDLE;
};

// =============================================================================
// RENDER ENGINE CLASS
// =============================================================================

/**
 * @brief Main Vulkan render engine with adaptive WeaR-Gen support
 */
class WeaR_RenderEngine {
public:
    using ErrorType = std::string;
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    WeaR_RenderEngine() = default;
    ~WeaR_RenderEngine();

    WeaR_RenderEngine(const WeaR_RenderEngine&) = delete;
    WeaR_RenderEngine& operator=(const WeaR_RenderEngine&) = delete;

    /**
     * @brief Initialize Vulkan with adaptive pipeline selection
     */
    [[nodiscard]] std::expected<void, ErrorType> initVulkan(
        const WeaR_Specs& specs,
        HWND windowHandle,
        const RenderEngineConfig& config = {}
    );

    void shutdown();

    [[nodiscard]] bool isInitialized() const { return m_initialized; }
    [[nodiscard]] bool isFrameGenActive() const { return m_frameGenActive; }

    bool setFrameGenEnabled(bool enabled);
    void setFrameGenParams(const FrameGenPushConstants& params);

    /**
     * @brief Render a frame with spinning triangle test
     */
    [[nodiscard]] std::expected<void, ErrorType> renderFrame();

    void onWindowResize(uint32_t width, uint32_t height);

    [[nodiscard]] VkInstance getInstance() const { return m_instance; }
    [[nodiscard]] VkDevice getDevice() const { return m_device; }
    [[nodiscard]] VkPhysicalDevice getPhysicalDevice() const { return m_physicalDevice; }
    [[nodiscard]] float getCurrentRotation() const { return m_rotationAngle; }

private:
    // Initialization stages
    std::expected<void, ErrorType> createInstance(const RenderEngineConfig& config);
    std::expected<void, ErrorType> selectPhysicalDevice();
    std::expected<void, ErrorType> createSurface(HWND windowHandle);
    std::expected<void, ErrorType> createLogicalDevice();
    std::expected<void, ErrorType> createVmaAllocator();
    std::expected<void, ErrorType> createSwapchain(uint32_t width, uint32_t height);
    std::expected<void, ErrorType> createSyncObjects();
    std::expected<void, ErrorType> createCommandPools();
    std::expected<void, ErrorType> createTrianglePipeline();
    std::expected<void, ErrorType> createFrameGenPipeline();
    std::expected<void, ErrorType> allocateFrameGenResources();
    std::expected<void, ErrorType> createVertexBuffer();

    // Helpers
    std::optional<uint32_t> findQueueFamily(VkQueueFlags flags, bool dedicated = false);
    AllocatedImage createImage(VkFormat format, VkExtent2D extent, VkImageUsageFlags usage);
    AllocatedBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage);
    void destroyImage(AllocatedImage& image);
    void destroyBuffer(AllocatedBuffer& buffer);
    VkShaderModule loadShaderModule(const std::string& path);
    VkShaderModule createShaderModuleFromSpirv(const std::vector<uint32_t>& spirv);
    void cleanupSwapchain();
    void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex);
    void transitionImageLayout(VkCommandBuffer cmd, VkImage image, 
                               VkImageLayout oldLayout, VkImageLayout newLayout);

    // Vulkan core handles
    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

    // Queues
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkQueue m_computeQueue = VK_NULL_HANDLE;
    uint32_t m_graphicsQueueFamily = UINT32_MAX;
    uint32_t m_presentQueueFamily = UINT32_MAX;
    uint32_t m_computeQueueFamily = UINT32_MAX;

    // VMA
    VmaAllocator m_allocator = nullptr;

    // Swapchain
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    VkFormat m_swapchainFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_swapchainExtent = {0, 0};

    // Command pools and buffers
    VkCommandPool m_graphicsCommandPool = VK_NULL_HANDLE;
    VkCommandPool m_computeCommandPool = VK_NULL_HANDLE;
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> m_graphicsCommandBuffers{};
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> m_computeCommandBuffers{};

    // Synchronization
    std::array<FrameSyncObjects, MAX_FRAMES_IN_FLIGHT> m_syncObjects{};
    uint32_t m_currentFrame = 0;

    // Triangle graphics pipeline
    VkPipelineLayout m_trianglePipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_trianglePipeline = VK_NULL_HANDLE;
    VkShaderModule m_triangleVertShader = VK_NULL_HANDLE;
    VkShaderModule m_triangleFragShader = VK_NULL_HANDLE;
    AllocatedBuffer m_vertexBuffer{};

    // WeaR-Gen compute pipeline
    VkPipelineLayout m_computePipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_computePipeline = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_computeDescriptorLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_computeDescriptorPool = VK_NULL_HANDLE;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> m_computeDescriptorSets{};
    VkShaderModule m_frameGenShader = VK_NULL_HANDLE;

    // Frame generation resources
    AllocatedImage m_prevFrame{};
    AllocatedImage m_currFrame{};
    AllocatedImage m_outputFrame{};
    FrameGenPushConstants m_frameGenParams{};

    // Animation state
    float m_rotationAngle = 0.0f;
    float m_prevRotationAngle = 0.0f;
    std::chrono::steady_clock::time_point m_lastFrameTime;

    // State
    bool m_initialized = false;
    bool m_frameGenActive = false;
    bool m_frameGenCapable = false;
    bool m_validationEnabled = false;
    WeaR_Specs m_specs{};
};

} // namespace WeaR
