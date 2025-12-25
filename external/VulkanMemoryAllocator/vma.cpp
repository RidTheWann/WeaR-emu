/**
 * @file vma.cpp
 * @brief VMA (Vulkan Memory Allocator) implementation file
 * 
 * This file triggers the compilation of the VMA implementation
 * by defining VMA_IMPLEMENTATION before including the header.
 */

// Platform defines for Vulkan
#if defined(_WIN32)
    #define VK_USE_PLATFORM_WIN32_KHR
#endif

#ifndef VK_NO_PROTOTYPES
    #define VK_NO_PROTOTYPES
#endif

// Include volk first for Vulkan function pointers
#include <volk.h>

// VMA implementation
#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <vk_mem_alloc.h>
