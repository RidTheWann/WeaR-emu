/**
 * Vulkan Memory Allocator (VMA) - Minimal Header Stub
 * 
 * This is a MINIMAL stub for compilation reference.
 * For production, clone the real VMA:
 *   git clone https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
 * 
 * Copy include/vk_mem_alloc.h to this location.
 */

#ifndef VK_MEM_ALLOC_H
#define VK_MEM_ALLOC_H

// NOTE: volk.h should be included before this header with VK_NO_PROTOTYPES defined

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// TYPE DEFINITIONS
// =============================================================================

typedef struct VmaAllocator_T* VmaAllocator;
typedef struct VmaAllocation_T* VmaAllocation;
typedef struct VmaPool_T* VmaPool;

typedef enum VmaMemoryUsage {
    VMA_MEMORY_USAGE_UNKNOWN = 0,
    VMA_MEMORY_USAGE_AUTO = 7,
    VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE = 8,
    VMA_MEMORY_USAGE_AUTO_PREFER_HOST = 9,
} VmaMemoryUsage;

typedef enum VmaAllocationCreateFlagBits {
    VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT = 0x00000001,
    VMA_ALLOCATION_CREATE_NEVER_ALLOCATE_BIT = 0x00000002,
    VMA_ALLOCATION_CREATE_MAPPED_BIT = 0x00000004,
    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT = 0x00000400,
    VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT = 0x00000800,
} VmaAllocationCreateFlagBits;
typedef VkFlags VmaAllocationCreateFlags;

typedef struct VmaVulkanFunctions {
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
    // ... other function pointers (simplified)
} VmaVulkanFunctions;

typedef struct VmaAllocatorCreateInfo {
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkInstance instance;
    uint32_t vulkanApiVersion;
    const VmaVulkanFunctions* pVulkanFunctions;
    // ... other fields (simplified)
} VmaAllocatorCreateInfo;

typedef struct VmaAllocationCreateInfo {
    VmaAllocationCreateFlags flags;
    VmaMemoryUsage usage;
    VkMemoryPropertyFlags requiredFlags;
    VkMemoryPropertyFlags preferredFlags;
    // ... other fields (simplified)
} VmaAllocationCreateInfo;

typedef struct VmaAllocationInfo {
    VkDeviceMemory deviceMemory;
    VkDeviceSize offset;
    VkDeviceSize size;
    void* pMappedData;
    // ... other fields
} VmaAllocationInfo;

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

VkResult vmaCreateAllocator(
    const VmaAllocatorCreateInfo* pCreateInfo,
    VmaAllocator* pAllocator);

void vmaDestroyAllocator(VmaAllocator allocator);

VkResult vmaCreateImage(
    VmaAllocator allocator,
    const VkImageCreateInfo* pImageCreateInfo,
    const VmaAllocationCreateInfo* pAllocationCreateInfo,
    VkImage* pImage,
    VmaAllocation* pAllocation,
    VmaAllocationInfo* pAllocationInfo);

void vmaDestroyImage(
    VmaAllocator allocator,
    VkImage image,
    VmaAllocation allocation);

VkResult vmaCreateBuffer(
    VmaAllocator allocator,
    const VkBufferCreateInfo* pBufferCreateInfo,
    const VmaAllocationCreateInfo* pAllocationCreateInfo,
    VkBuffer* pBuffer,
    VmaAllocation* pAllocation,
    VmaAllocationInfo* pAllocationInfo);

void vmaDestroyBuffer(
    VmaAllocator allocator,
    VkBuffer buffer,
    VmaAllocation allocation);

VkResult vmaMapMemory(
    VmaAllocator allocator,
    VmaAllocation allocation,
    void** ppData);

void vmaUnmapMemory(
    VmaAllocator allocator,
    VmaAllocation allocation);

void vmaGetAllocationInfo(
    VmaAllocator allocator,
    VmaAllocation allocation,
    VmaAllocationInfo* pAllocationInfo);

#ifdef __cplusplus
}
#endif

// =============================================================================
// IMPLEMENTATION (when VMA_IMPLEMENTATION is defined)
// =============================================================================
#ifdef VMA_IMPLEMENTATION

// This stub implementation just returns success
// Replace with real VMA for actual memory management

VkResult vmaCreateAllocator(
    const VmaAllocatorCreateInfo* pCreateInfo,
    VmaAllocator* pAllocator)
{
    (void)pCreateInfo;
    // Allocate dummy handle
    *pAllocator = (VmaAllocator)1;
    return VK_SUCCESS;
}

void vmaDestroyAllocator(VmaAllocator allocator)
{
    (void)allocator;
}

VkResult vmaCreateImage(
    VmaAllocator allocator,
    const VkImageCreateInfo* pImageCreateInfo,
    const VmaAllocationCreateInfo* pAllocationCreateInfo,
    VkImage* pImage,
    VmaAllocation* pAllocation,
    VmaAllocationInfo* pAllocationInfo)
{
    (void)allocator;
    (void)pAllocationCreateInfo;
    (void)pAllocationInfo;
    
    // This stub does NOT actually work - replace with real VMA
    // For now, fail gracefully to indicate VMA is needed
    *pImage = VK_NULL_HANDLE;
    *pAllocation = nullptr;
    return VK_ERROR_INITIALIZATION_FAILED;
}

void vmaDestroyImage(
    VmaAllocator allocator,
    VkImage image,
    VmaAllocation allocation)
{
    (void)allocator;
    (void)image;
    (void)allocation;
}

VkResult vmaCreateBuffer(
    VmaAllocator allocator,
    const VkBufferCreateInfo* pBufferCreateInfo,
    const VmaAllocationCreateInfo* pAllocationCreateInfo,
    VkBuffer* pBuffer,
    VmaAllocation* pAllocation,
    VmaAllocationInfo* pAllocationInfo)
{
    (void)allocator;
    (void)pBufferCreateInfo;
    (void)pAllocationCreateInfo;
    (void)pAllocationInfo;
    *pBuffer = VK_NULL_HANDLE;
    *pAllocation = nullptr;
    return VK_ERROR_INITIALIZATION_FAILED;
}

void vmaDestroyBuffer(
    VmaAllocator allocator,
    VkBuffer buffer,
    VmaAllocation allocation)
{
    (void)allocator;
    (void)buffer;
    (void)allocation;
}

VkResult vmaMapMemory(
    VmaAllocator allocator,
    VmaAllocation allocation,
    void** ppData)
{
    (void)allocator;
    (void)allocation;
    *ppData = nullptr;
    return VK_ERROR_MEMORY_MAP_FAILED;
}

void vmaUnmapMemory(
    VmaAllocator allocator,
    VmaAllocation allocation)
{
    (void)allocator;
    (void)allocation;
}

void vmaGetAllocationInfo(
    VmaAllocator allocator,
    VmaAllocation allocation,
    VmaAllocationInfo* pAllocationInfo)
{
    (void)allocator;
    (void)allocation;
    if (pAllocationInfo) {
        pAllocationInfo->pMappedData = nullptr;
        pAllocationInfo->size = 0;
        pAllocationInfo->offset = 0;
        pAllocationInfo->deviceMemory = VK_NULL_HANDLE;
    }
}

#endif // VMA_IMPLEMENTATION

#endif // VK_MEM_ALLOC_H
