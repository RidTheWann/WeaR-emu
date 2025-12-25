#include "WeaR_ShaderManager.h"

// --- VULKAN INCLUDES (ORDER IS CRITICAL) ---
#if defined(_WIN32)
    #define VK_USE_PLATFORM_WIN32_KHR
#endif

#ifndef VK_NO_PROTOTYPES
    #define VK_NO_PROTOTYPES
#endif

#include <volk.h>
// -------------------------------------------

#include <iostream>
#include <format>
#include <cstring>

namespace WeaR {

// =============================================================================
// GLOBAL INSTANCE
// =============================================================================

WeaR_ShaderManager& getShaderManager() {
    static WeaR_ShaderManager instance;
    return instance;
}

// =============================================================================
// EMBEDDED FALLBACK SHADERS (SPIR-V)
// =============================================================================

// Simple fallback vertex shader (position passthrough with push constants)
// Compiled from simplified GLSL to work without external files
const std::vector<uint32_t> FALLBACK_VERT_SPIRV = {
    // SPIR-V 1.0, Bound 60
    0x07230203, 0x00010000, 0x000d000a, 0x0000003c,
    0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
    0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x000b000f, 0x00000000, 0x00000004, 0x6e69616d,
    0x00000000, 0x0000000d, 0x00000012, 0x00000020,
    0x00000024, 0x0000002a, 0x0000002e, 0x00030003,
    0x00000002, 0x000001c2, 0x00040005, 0x00000004,
    0x6e69616d, 0x00000000, 0x00060005, 0x0000000b,
    0x505f6c67, 0x65567265, 0x78657472, 0x00000000,
    0x00050005, 0x0000000d, 0x6f506e69, 0x69746973,
    0x00006e6f, 0x00050005, 0x00000012, 0x67617266,
    0x6f6c6f43, 0x00000072, 0x00040048, 0x00000009,
    0x00000000, 0x00000005, 0x00050048, 0x00000009,
    0x00000000, 0x00000023, 0x00000000, 0x00050048,
    0x00000009, 0x00000000, 0x00000007, 0x00000010,
    0x00030047, 0x00000009, 0x00000002, 0x00040047,
    0x0000000d, 0x0000001e, 0x00000000, 0x00040047,
    0x00000012, 0x0000001e, 0x00000000, 0x00020013,
    0x00000002, 0x00030021, 0x00000003, 0x00000002,
    0x00030016, 0x00000006, 0x00000020, 0x00040017,
    0x00000007, 0x00000006, 0x00000004, 0x00040018,
    0x00000008, 0x00000007, 0x00000004, 0x0003001e,
    0x00000009, 0x00000008, 0x00040020, 0x0000000a,
    0x00000009, 0x00000009, 0x0004003b, 0x0000000a,
    0x0000000b, 0x00000009, 0x00040017, 0x0000000c,
    0x00000006, 0x00000003, 0x00040020, 0x0000000e,
    0x00000001, 0x0000000c, 0x0004003b, 0x0000000e,
    0x0000000d, 0x00000001, 0x00040020, 0x00000010,
    0x00000003, 0x0000000c, 0x0004003b, 0x00000010,
    0x00000012, 0x00000003, 0x00040020, 0x00000014,
    0x00000003, 0x00000007, 0x0004003b, 0x00000014,
    0x00000020, 0x00000003, 0x00050036, 0x00000002,
    0x00000004, 0x00000000, 0x00000003, 0x000200f8,
    0x00000005, 0x0004003d, 0x0000000c, 0x00000015,
    0x0000000d, 0x00050051, 0x00000006, 0x00000016,
    0x00000015, 0x00000000, 0x00050051, 0x00000006,
    0x00000017, 0x00000015, 0x00000001, 0x00050051,
    0x00000006, 0x00000018, 0x00000015, 0x00000002,
    0x0004002b, 0x00000006, 0x00000019, 0x3f800000,
    0x00070050, 0x00000007, 0x0000001a, 0x00000016,
    0x00000017, 0x00000018, 0x00000019, 0x0003003e,
    0x00000020, 0x0000001a, 0x0003003e, 0x00000012,
    0x00000015, 0x000100fd, 0x00010038
};

// Simple fallback fragment shader (neon magenta output)
const std::vector<uint32_t> FALLBACK_FRAG_SPIRV = {
    // SPIR-V 1.0
    0x07230203, 0x00010000, 0x000d000a, 0x00000018,
    0x00000000, 0x00020011, 0x00000001, 0x0006000b,
    0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
    0x00000000, 0x0003000e, 0x00000000, 0x00000001,
    0x0007000f, 0x00000004, 0x00000004, 0x6e69616d,
    0x00000000, 0x00000009, 0x0000000c, 0x00030010,
    0x00000004, 0x00000007, 0x00030003, 0x00000002,
    0x000001c2, 0x00040005, 0x00000004, 0x6e69616d,
    0x00000000, 0x00050005, 0x00000009, 0x4374756f,
    0x726f6c6f, 0x00000000, 0x00050005, 0x0000000c,
    0x67617266, 0x6f6c6f43, 0x00000072, 0x00040047,
    0x00000009, 0x0000001e, 0x00000000, 0x00040047,
    0x0000000c, 0x0000001e, 0x00000000, 0x00020013,
    0x00000002, 0x00030021, 0x00000003, 0x00000002,
    0x00030016, 0x00000006, 0x00000020, 0x00040017,
    0x00000007, 0x00000006, 0x00000004, 0x00040020,
    0x00000008, 0x00000003, 0x00000007, 0x0004003b,
    0x00000008, 0x00000009, 0x00000003, 0x00040017,
    0x0000000a, 0x00000006, 0x00000003, 0x00040020,
    0x0000000b, 0x00000001, 0x0000000a, 0x0004003b,
    0x0000000b, 0x0000000c, 0x00000001, 0x0004002b,
    0x00000006, 0x0000000e, 0x3f800000, 0x0004002b,
    0x00000006, 0x0000000f, 0x00000000, 0x0004002b,
    0x00000006, 0x00000010, 0x3f19999a, 0x00050036,
    0x00000002, 0x00000004, 0x00000000, 0x00000003,
    0x000200f8, 0x00000005, 0x0004003d, 0x0000000a,
    0x0000000d, 0x0000000c, 0x00050051, 0x00000006,
    0x00000011, 0x0000000d, 0x00000000, 0x00050051,
    0x00000006, 0x00000012, 0x0000000d, 0x00000001,
    0x00050051, 0x00000006, 0x00000013, 0x0000000d,
    0x00000002, 0x00050085, 0x00000006, 0x00000014,
    0x00000011, 0x0000000e, 0x00060050, 0x00000007,
    0x00000015, 0x0000000e, 0x00000010, 0x00000014,
    0x00050051, 0x00000006, 0x00000016, 0x00000015,
    0x00000002, 0x00070050, 0x00000007, 0x00000017,
    0x0000000e, 0x00000010, 0x00000016, 0x0000000e,
    0x0003003e, 0x00000009, 0x00000017, 0x000100fd,
    0x00010038
};

// =============================================================================
// DESTRUCTOR
// =============================================================================

WeaR_ShaderManager::~WeaR_ShaderManager() {
    shutdown();
}

// =============================================================================
// INITIALIZATION
// =============================================================================

std::expected<void, WeaR_ShaderManager::ErrorType> WeaR_ShaderManager::init(
    VkDevice device,
    VkFormat swapchainFormat)
{
    if (m_initialized) {
        return std::unexpected("ShaderManager already initialized");
    }

    m_device = device;
    m_swapchainFormat = swapchainFormat;

    std::cout << "[ShaderManager] Initializing...\n";

    if (auto r = createFallbackShaders(); !r) {
        return r;
    }

    if (auto r = createFallbackPipeline(); !r) {
        return r;
    }

    // Initialize push constants with identity matrix
    std::memset(&m_pushConstants, 0, sizeof(m_pushConstants));
    // Identity matrix
    m_pushConstants.mvp[0] = 1.0f;
    m_pushConstants.mvp[5] = 1.0f;
    m_pushConstants.mvp[10] = 1.0f;
    m_pushConstants.mvp[15] = 1.0f;
    // Default debug color (magenta)
    m_pushConstants.debugColor[0] = 1.0f;  // R
    m_pushConstants.debugColor[1] = 0.0f;  // G
    m_pushConstants.debugColor[2] = 0.6f;  // B
    m_pushConstants.debugColor[3] = 1.0f;  // A

    m_initialized = true;
    std::cout << "[ShaderManager] Initialized with fallback pipeline\n";
    return {};
}

void WeaR_ShaderManager::shutdown() {
    if (!m_device) return;

    // Cleanup cached pipelines
    for (auto& [state, pipeline] : m_pipelineCache) {
        if (pipeline) vkDestroyPipeline(m_device, pipeline, nullptr);
    }
    m_pipelineCache.clear();

    if (m_wireframePipeline) vkDestroyPipeline(m_device, m_wireframePipeline, nullptr);
    if (m_fallbackPipeline) vkDestroyPipeline(m_device, m_fallbackPipeline, nullptr);
    if (m_fallbackLayout) vkDestroyPipelineLayout(m_device, m_fallbackLayout, nullptr);
    if (m_fallbackFragShader) vkDestroyShaderModule(m_device, m_fallbackFragShader, nullptr);
    if (m_fallbackVertShader) vkDestroyShaderModule(m_device, m_fallbackVertShader, nullptr);

    m_fallbackPipeline = VK_NULL_HANDLE;
    m_fallbackLayout = VK_NULL_HANDLE;
    m_initialized = false;
}

// =============================================================================
// PIPELINE ACCESS
// =============================================================================

VkPipeline WeaR_ShaderManager::getPipeline(const WeaR_PipelineState& state) {
    if (!m_initialized) return VK_NULL_HANDLE;

    // Check cache first
    auto it = m_pipelineCache.find(state);
    if (it != m_pipelineCache.end()) {
        return it->second;
    }

    // Future: Check if we have translated SPIR-V for this PS4 shader
    // For now, ALWAYS return fallback

    // Cache the fallback pipeline for this state
    m_pipelineCache[state] = m_fallbackPipeline;
    
    return m_renderMode == RenderMode::Wireframe ? m_wireframePipeline : m_fallbackPipeline;
}

void WeaR_ShaderManager::updatePushConstants(float time) {
    m_pushConstants.time = time;
}

// =============================================================================
// SHADER CREATION
// =============================================================================

std::expected<void, WeaR_ShaderManager::ErrorType> WeaR_ShaderManager::createFallbackShaders() {
    m_fallbackVertShader = createShaderModuleFromSpirv(FALLBACK_VERT_SPIRV);
    if (!m_fallbackVertShader) {
        return std::unexpected("Failed to create fallback vertex shader");
    }

    m_fallbackFragShader = createShaderModuleFromSpirv(FALLBACK_FRAG_SPIRV);
    if (!m_fallbackFragShader) {
        return std::unexpected("Failed to create fallback fragment shader");
    }

    return {};
}

VkShaderModule WeaR_ShaderManager::createShaderModuleFromSpirv(const std::vector<uint32_t>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}

std::expected<void, WeaR_ShaderManager::ErrorType> WeaR_ShaderManager::createFallbackPipeline() {
    // Pipeline layout with push constants
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(FallbackPushConstants);

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &pushRange;

    if (vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_fallbackLayout) != VK_SUCCESS) {
        return std::unexpected("Failed to create fallback pipeline layout");
    }

    // Shader stages
    VkPipelineShaderStageCreateInfo vertStage{};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = m_fallbackVertShader;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage{};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = m_fallbackFragShader;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = { vertStage, fragStage };

    // Vertex input (position + color passthrough)
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(float) * 6;  // vec3 pos + vec3 color/normal
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[2]{};
    attrs[0].location = 0;
    attrs[0].binding = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset = 0;
    attrs[1].location = 1;
    attrs[1].binding = 0;
    attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset = sizeof(float) * 3;

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;  // No culling for debug
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // Dynamic rendering (Vulkan 1.3)
    VkPipelineRenderingCreateInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &m_swapchainFormat;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pNext = &renderingInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_fallbackLayout;

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_fallbackPipeline) != VK_SUCCESS) {
        return std::unexpected("Failed to create fallback pipeline");
    }

    // Create wireframe variant
    rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_wireframePipeline) != VK_SUCCESS) {
        // Non-fatal, wireframe not available on all devices
        m_wireframePipeline = m_fallbackPipeline;
    }

    return {};
}

} // namespace WeaR
