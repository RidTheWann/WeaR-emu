#pragma once

/**
 * @file WeaR_VulkanWrapper.h
 * @brief Single source of truth for Vulkan includes
 * 
 * All Vulkan-related files MUST include this header instead of
 * including volk.h or vk_mem_alloc.h directly.
 * 
 * VK_NO_PROTOTYPES is defined globally by CMake.
 */

// 1. Platform definitions
#if defined(_WIN32)
    #define VK_USE_PLATFORM_WIN32_KHR
#endif

// 2. Include Volk (Dynamic Vulkan Loader) - MUST BE FIRST
#include <volk.h>

// 3. Include VMA (Vulkan Memory Allocator) - AFTER volk
#include <vk_mem_alloc.h>
