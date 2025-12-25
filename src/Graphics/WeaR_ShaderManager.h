#pragma once

/**
 * @file WeaR_ShaderManager.h
 * @brief Shader management with fallback pipeline for untranslated PS4 shaders
 * 
 * Manages shader resources and provides a "fallback" rendering mode when
 * native PS4 GCN shaders are not yet recompiled to SPIR-V.
 */

#include "WeaR_RenderQueue.h"

// Vulkan forward declarations (minimal header)
#if defined(_WIN32)
    #define VK_USE_PLATFORM_WIN32_KHR
#endif
#define VK_NO_PROTOTYPES
#include <volk.h>

#include <unordered_map>
#include <memory>
#include <string>
#include <expected>
#include <cstdint>
#include <functional>

namespace WeaR {

// =============================================================================
// PIPELINE CONFIGURATION
// =============================================================================

/**
 * @brief Hash for PipelineState (for unordered_map key)
 */
struct PipelineStateHash {
    std::size_t operator()(const WeaR_PipelineState& state) const {
        std::size_t h1 = std::hash<uint64_t>{}(state.vsShaderAddr);
        std::size_t h2 = std::hash<uint64_t>{}(state.psShaderAddr);
        std::size_t h3 = std::hash<uint32_t>{}(state.primitiveType);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

/**
 * @brief Push constants for fallback shader
 */
struct FallbackPushConstants {
    float mvp[16];          // 4x4 MVP matrix (identity for now)
    float debugColor[4];    // Debug color RGBA
    float time;             // Animation time
    float padding[3];       // Alignment
};

/**
 * @brief Render mode for debugging
 */
enum class RenderMode : uint8_t {
    Solid,      // Filled polygons
    Wireframe,  // Edge lines only
    Points      // Vertices only
};

// =============================================================================
// SHADER MANAGER CLASS
// =============================================================================

/**
 * @brief Manages shaders and pipelines with fallback support
 */
class WeaR_ShaderManager {
public:
    using ErrorType = std::string;

    WeaR_ShaderManager() = default;
    ~WeaR_ShaderManager();

    // Non-copyable
    WeaR_ShaderManager(const WeaR_ShaderManager&) = delete;
    WeaR_ShaderManager& operator=(const WeaR_ShaderManager&) = delete;

    /**
     * @brief Initialize shader manager
     * @param device Vulkan logical device
     * @param swapchainFormat Swapchain image format
     * @return Success or error
     */
    [[nodiscard]] std::expected<void, ErrorType> init(
        VkDevice device,
        VkFormat swapchainFormat
    );

    /**
     * @brief Shutdown and cleanup
     */
    void shutdown();

    /**
     * @brief Get pipeline for given state (returns fallback if not available)
     * @param state Requested pipeline state
     * @return VkPipeline (fallback if PS4 shader not translated)
     */
    [[nodiscard]] VkPipeline getPipeline(const WeaR_PipelineState& state);

    /**
     * @brief Get fallback pipeline layout
     */
    [[nodiscard]] VkPipelineLayout getFallbackLayout() const { return m_fallbackLayout; }

    /**
     * @brief Get fallback pipeline directly
     */
    [[nodiscard]] VkPipeline getFallbackPipeline() const { return m_fallbackPipeline; }

    /**
     * @brief Update push constants
     */
    void updatePushConstants(float time);

    /**
     * @brief Get current push constants
     */
    [[nodiscard]] const FallbackPushConstants& getPushConstants() const { return m_pushConstants; }

    /**
     * @brief Set render mode
     */
    void setRenderMode(RenderMode mode) { m_renderMode = mode; }
    [[nodiscard]] RenderMode getRenderMode() const { return m_renderMode; }

    /**
     * @brief Check if initialized
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    /**
     * @brief Statistics
     */
    [[nodiscard]] uint32_t getCachedPipelineCount() const { 
        return static_cast<uint32_t>(m_pipelineCache.size()); 
    }

private:
    [[nodiscard]] std::expected<void, ErrorType> createFallbackShaders();
    [[nodiscard]] std::expected<void, ErrorType> createFallbackPipeline();
    [[nodiscard]] VkShaderModule createShaderModuleFromSpirv(const std::vector<uint32_t>& code);
    
    VkDevice m_device = VK_NULL_HANDLE;
    VkFormat m_swapchainFormat = VK_FORMAT_UNDEFINED;
    bool m_initialized = false;
    RenderMode m_renderMode = RenderMode::Solid;

    // Fallback shaders and pipeline
    VkShaderModule m_fallbackVertShader = VK_NULL_HANDLE;
    VkShaderModule m_fallbackFragShader = VK_NULL_HANDLE;
    VkPipelineLayout m_fallbackLayout = VK_NULL_HANDLE;
    VkPipeline m_fallbackPipeline = VK_NULL_HANDLE;
    VkPipeline m_wireframePipeline = VK_NULL_HANDLE;

    // Pipeline cache
    std::unordered_map<WeaR_PipelineState, VkPipeline, PipelineStateHash> m_pipelineCache;

    // Push constants
    FallbackPushConstants m_pushConstants{};
};

/**
 * @brief Get global shader manager instance
 */
WeaR_ShaderManager& getShaderManager();

} // namespace WeaR
