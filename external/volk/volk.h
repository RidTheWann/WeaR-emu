/**
 * @file volk.h  
 * @brief Volk - Vulkan Meta-Loader (Header-Only)
 * 
 * This is a MINIMAL stub for compilation. For production use, clone the real volk:
 *   git clone https://github.com/zeux/volk.git
 * 
 * Volk provides dynamic loading of Vulkan functions, eliminating the need
 * to link against the Vulkan loader at compile time.
 */

#ifndef VOLK_H_
#define VOLK_H_

// Disable Vulkan prototypes - we load them dynamically
#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif

#include <vulkan/vulkan.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize volk and load global Vulkan functions
 * @return VK_SUCCESS on success, error code otherwise
 */
VkResult volkInitialize(void);

/**
 * @brief Load instance-level Vulkan functions
 * @param instance The Vulkan instance
 */
void volkLoadInstance(VkInstance instance);

/**
 * @brief Load device-level Vulkan functions
 * @param device The Vulkan device
 */
void volkLoadDevice(VkDevice device);

// =============================================================================
// EXTERN DECLARATIONS (visible to all translation units)
// =============================================================================

// Global Vulkan functions
extern PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
extern PFN_vkCreateInstance vkCreateInstance;
extern PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties;
extern PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;

// Instance-level functions
extern PFN_vkDestroyInstance vkDestroyInstance;
extern PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
extern PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
extern PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2;
extern PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures;
extern PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2;
extern PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
extern PFN_vkGetPhysicalDeviceMemoryProperties2 vkGetPhysicalDeviceMemoryProperties2;
extern PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
extern PFN_vkCreateDevice vkCreateDevice;
extern PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
extern PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;

// Device-level functions
extern PFN_vkDestroyDevice vkDestroyDevice;
extern PFN_vkGetDeviceQueue vkGetDeviceQueue;
extern PFN_vkDeviceWaitIdle vkDeviceWaitIdle;
extern PFN_vkCreateShaderModule vkCreateShaderModule;
extern PFN_vkDestroyShaderModule vkDestroyShaderModule;
extern PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
extern PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
extern PFN_vkCreateComputePipelines vkCreateComputePipelines;
extern PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines;
extern PFN_vkDestroyPipeline vkDestroyPipeline;
extern PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout;
extern PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout;
extern PFN_vkCreateDescriptorPool vkCreateDescriptorPool;
extern PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool;
extern PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets;
extern PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets;
extern PFN_vkCreateCommandPool vkCreateCommandPool;
extern PFN_vkDestroyCommandPool vkDestroyCommandPool;
extern PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
extern PFN_vkFreeCommandBuffers vkFreeCommandBuffers;
extern PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
extern PFN_vkEndCommandBuffer vkEndCommandBuffer;
extern PFN_vkQueueSubmit vkQueueSubmit;
extern PFN_vkQueueWaitIdle vkQueueWaitIdle;
extern PFN_vkCreateFence vkCreateFence;
extern PFN_vkDestroyFence vkDestroyFence;
extern PFN_vkWaitForFences vkWaitForFences;
extern PFN_vkResetFences vkResetFences;
extern PFN_vkCreateSemaphore vkCreateSemaphore;
extern PFN_vkDestroySemaphore vkDestroySemaphore;
extern PFN_vkCreateImageView vkCreateImageView;
extern PFN_vkDestroyImageView vkDestroyImageView;
extern PFN_vkCreateImage vkCreateImage;
extern PFN_vkDestroyImage vkDestroyImage;
extern PFN_vkCreateBuffer vkCreateBuffer;
extern PFN_vkDestroyBuffer vkDestroyBuffer;
extern PFN_vkAllocateMemory vkAllocateMemory;
extern PFN_vkFreeMemory vkFreeMemory;
extern PFN_vkBindBufferMemory vkBindBufferMemory;
extern PFN_vkBindImageMemory vkBindImageMemory;
extern PFN_vkMapMemory vkMapMemory;
extern PFN_vkUnmapMemory vkUnmapMemory;
extern PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements;
extern PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
extern PFN_vkCmdBeginRendering vkCmdBeginRendering;
extern PFN_vkCmdEndRendering vkCmdEndRendering;
extern PFN_vkCmdBindPipeline vkCmdBindPipeline;
extern PFN_vkCmdSetViewport vkCmdSetViewport;
extern PFN_vkCmdSetScissor vkCmdSetScissor;
extern PFN_vkCmdDraw vkCmdDraw;
extern PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers;
extern PFN_vkCmdCopyBuffer vkCmdCopyBuffer;
extern PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
extern PFN_vkCmdPushConstants vkCmdPushConstants;
extern PFN_vkResetCommandBuffer vkResetCommandBuffer;

// Swapchain
extern PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
extern PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
extern PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
extern PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
extern PFN_vkQueuePresentKHR vkQueuePresentKHR;

// Surface
extern PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
extern PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
extern PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
extern PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
extern PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;

#ifdef VK_USE_PLATFORM_WIN32_KHR
extern PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
#endif

// Debug utils
extern PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
extern PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;

#ifdef __cplusplus
}
#endif

// =============================================================================
// IMPLEMENTATION (when VOLK_IMPLEMENTATION is defined)
// =============================================================================
#ifdef VOLK_IMPLEMENTATION

#ifdef _WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

// Function pointer storage (internal)
static PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr_volkImpl = NULL;

// Global Vulkan functions (loaded by volkInitialize)
PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = NULL;
PFN_vkCreateInstance vkCreateInstance = NULL;
PFN_vkEnumerateInstanceExtensionProperties vkEnumerateInstanceExtensionProperties = NULL;
PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties = NULL;

// Instance-level functions (loaded by volkLoadInstance)
PFN_vkDestroyInstance vkDestroyInstance = NULL;
PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = NULL;
PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties = NULL;
PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2 = NULL;
PFN_vkGetPhysicalDeviceFeatures vkGetPhysicalDeviceFeatures = NULL;
PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2 = NULL;
PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties = NULL;
PFN_vkGetPhysicalDeviceMemoryProperties2 vkGetPhysicalDeviceMemoryProperties2 = NULL;
PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties = NULL;
PFN_vkCreateDevice vkCreateDevice = NULL;
PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = NULL;
PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties = NULL;

// Device-level functions (loaded by volkLoadDevice)
PFN_vkDestroyDevice vkDestroyDevice = NULL;
PFN_vkGetDeviceQueue vkGetDeviceQueue = NULL;
PFN_vkDeviceWaitIdle vkDeviceWaitIdle = NULL;
PFN_vkCreateShaderModule vkCreateShaderModule = NULL;
PFN_vkDestroyShaderModule vkDestroyShaderModule = NULL;
PFN_vkCreatePipelineLayout vkCreatePipelineLayout = NULL;
PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout = NULL;
PFN_vkCreateComputePipelines vkCreateComputePipelines = NULL;
PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines = NULL;
PFN_vkDestroyPipeline vkDestroyPipeline = NULL;
PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout = NULL;
PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout = NULL;
PFN_vkCreateDescriptorPool vkCreateDescriptorPool = NULL;
PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool = NULL;
PFN_vkAllocateDescriptorSets vkAllocateDescriptorSets = NULL;
PFN_vkUpdateDescriptorSets vkUpdateDescriptorSets = NULL;
PFN_vkCreateCommandPool vkCreateCommandPool = NULL;
PFN_vkDestroyCommandPool vkDestroyCommandPool = NULL;
PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers = NULL;
PFN_vkFreeCommandBuffers vkFreeCommandBuffers = NULL;
PFN_vkBeginCommandBuffer vkBeginCommandBuffer = NULL;
PFN_vkEndCommandBuffer vkEndCommandBuffer = NULL;
PFN_vkQueueSubmit vkQueueSubmit = NULL;
PFN_vkQueueWaitIdle vkQueueWaitIdle = NULL;
PFN_vkCreateFence vkCreateFence = NULL;
PFN_vkDestroyFence vkDestroyFence = NULL;
PFN_vkWaitForFences vkWaitForFences = NULL;
PFN_vkResetFences vkResetFences = NULL;
PFN_vkCreateSemaphore vkCreateSemaphore = NULL;
PFN_vkDestroySemaphore vkDestroySemaphore = NULL;
PFN_vkCreateImageView vkCreateImageView = NULL;
PFN_vkDestroyImageView vkDestroyImageView = NULL;
PFN_vkCreateImage vkCreateImage = NULL;
PFN_vkDestroyImage vkDestroyImage = NULL;
PFN_vkCreateBuffer vkCreateBuffer = NULL;
PFN_vkDestroyBuffer vkDestroyBuffer = NULL;
PFN_vkAllocateMemory vkAllocateMemory = NULL;
PFN_vkFreeMemory vkFreeMemory = NULL;
PFN_vkBindBufferMemory vkBindBufferMemory = NULL;
PFN_vkBindImageMemory vkBindImageMemory = NULL;
PFN_vkMapMemory vkMapMemory = NULL;
PFN_vkUnmapMemory vkUnmapMemory = NULL;
PFN_vkGetImageMemoryRequirements vkGetImageMemoryRequirements = NULL;
PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements = NULL;
PFN_vkCmdBeginRendering vkCmdBeginRendering = NULL;
PFN_vkCmdEndRendering vkCmdEndRendering = NULL;
PFN_vkCmdBindPipeline vkCmdBindPipeline = NULL;
PFN_vkCmdSetViewport vkCmdSetViewport = NULL;
PFN_vkCmdSetScissor vkCmdSetScissor = NULL;
PFN_vkCmdDraw vkCmdDraw = NULL;
PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers = NULL;
PFN_vkCmdCopyBuffer vkCmdCopyBuffer = NULL;
PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier = NULL;
PFN_vkCmdPushConstants vkCmdPushConstants = NULL;
PFN_vkResetCommandBuffer vkResetCommandBuffer = NULL;

// Swapchain
PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = NULL;
PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR = NULL;
PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = NULL;
PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR = NULL;
PFN_vkQueuePresentKHR vkQueuePresentKHR = NULL;

// Surface
PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR = NULL;
PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR = NULL;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR = NULL;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR = NULL;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR = NULL;

#ifdef VK_USE_PLATFORM_WIN32_KHR
PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = NULL;
#endif

// Debug utils
PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = NULL;
PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = NULL;

VkResult volkInitialize(void) {
#ifdef _WIN32
    HMODULE vulkanLib = LoadLibraryA("vulkan-1.dll");
    if (!vulkanLib) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    vkGetInstanceProcAddr_volkImpl = (PFN_vkGetInstanceProcAddr)GetProcAddress(vulkanLib, "vkGetInstanceProcAddr");
#else
    void* vulkanLib = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
    if (!vulkanLib) {
        vulkanLib = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
    }
    if (!vulkanLib) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    vkGetInstanceProcAddr_volkImpl = (PFN_vkGetInstanceProcAddr)dlsym(vulkanLib, "vkGetInstanceProcAddr");
#endif

    if (!vkGetInstanceProcAddr_volkImpl) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    
    // Set public vkGetInstanceProcAddr
    vkGetInstanceProcAddr = vkGetInstanceProcAddr_volkImpl;

    // Load global functions
    vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr_volkImpl(NULL, "vkCreateInstance");
    vkEnumerateInstanceExtensionProperties = (PFN_vkEnumerateInstanceExtensionProperties)vkGetInstanceProcAddr_volkImpl(NULL, "vkEnumerateInstanceExtensionProperties");
    vkEnumerateInstanceLayerProperties = (PFN_vkEnumerateInstanceLayerProperties)vkGetInstanceProcAddr_volkImpl(NULL, "vkEnumerateInstanceLayerProperties");

    if (!vkCreateInstance || !vkEnumerateInstanceExtensionProperties) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    return VK_SUCCESS;
}

void volkLoadInstance(VkInstance instance) {
    if (!instance || !vkGetInstanceProcAddr_volkImpl) return;

#define LOAD_INSTANCE_FUNC(name) name = (PFN_##name)vkGetInstanceProcAddr_volkImpl(instance, #name)

    LOAD_INSTANCE_FUNC(vkDestroyInstance);
    LOAD_INSTANCE_FUNC(vkEnumeratePhysicalDevices);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceProperties);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceProperties2);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceFeatures);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceFeatures2);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceMemoryProperties);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceMemoryProperties2);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceQueueFamilyProperties);
    LOAD_INSTANCE_FUNC(vkCreateDevice);
    LOAD_INSTANCE_FUNC(vkGetDeviceProcAddr);
    LOAD_INSTANCE_FUNC(vkEnumerateDeviceExtensionProperties);
    
    // Surface extension
    LOAD_INSTANCE_FUNC(vkDestroySurfaceKHR);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceSupportKHR);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR);
    LOAD_INSTANCE_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR);
    
#ifdef VK_USE_PLATFORM_WIN32_KHR
    LOAD_INSTANCE_FUNC(vkCreateWin32SurfaceKHR);
#endif
    
    // Debug utils (may not be present)
    LOAD_INSTANCE_FUNC(vkCreateDebugUtilsMessengerEXT);
    LOAD_INSTANCE_FUNC(vkDestroyDebugUtilsMessengerEXT);

#undef LOAD_INSTANCE_FUNC
}

void volkLoadDevice(VkDevice device) {
    if (!device || !vkGetDeviceProcAddr) return;

#define LOAD_DEVICE_FUNC(name) name = (PFN_##name)vkGetDeviceProcAddr(device, #name)

    LOAD_DEVICE_FUNC(vkDestroyDevice);
    LOAD_DEVICE_FUNC(vkGetDeviceQueue);
    LOAD_DEVICE_FUNC(vkDeviceWaitIdle);
    LOAD_DEVICE_FUNC(vkCreateShaderModule);
    LOAD_DEVICE_FUNC(vkDestroyShaderModule);
    LOAD_DEVICE_FUNC(vkCreatePipelineLayout);
    LOAD_DEVICE_FUNC(vkDestroyPipelineLayout);
    LOAD_DEVICE_FUNC(vkCreateComputePipelines);
    LOAD_DEVICE_FUNC(vkCreateGraphicsPipelines);
    LOAD_DEVICE_FUNC(vkDestroyPipeline);
    LOAD_DEVICE_FUNC(vkCreateDescriptorSetLayout);
    LOAD_DEVICE_FUNC(vkDestroyDescriptorSetLayout);
    LOAD_DEVICE_FUNC(vkCreateDescriptorPool);
    LOAD_DEVICE_FUNC(vkDestroyDescriptorPool);
    LOAD_DEVICE_FUNC(vkAllocateDescriptorSets);
    LOAD_DEVICE_FUNC(vkUpdateDescriptorSets);
    LOAD_DEVICE_FUNC(vkCreateCommandPool);
    LOAD_DEVICE_FUNC(vkDestroyCommandPool);
    LOAD_DEVICE_FUNC(vkAllocateCommandBuffers);
    LOAD_DEVICE_FUNC(vkFreeCommandBuffers);
    LOAD_DEVICE_FUNC(vkBeginCommandBuffer);
    LOAD_DEVICE_FUNC(vkEndCommandBuffer);
    LOAD_DEVICE_FUNC(vkQueueSubmit);
    LOAD_DEVICE_FUNC(vkQueueWaitIdle);
    LOAD_DEVICE_FUNC(vkCreateFence);
    LOAD_DEVICE_FUNC(vkDestroyFence);
    LOAD_DEVICE_FUNC(vkWaitForFences);
    LOAD_DEVICE_FUNC(vkResetFences);
    LOAD_DEVICE_FUNC(vkCreateSemaphore);
    LOAD_DEVICE_FUNC(vkDestroySemaphore);
    LOAD_DEVICE_FUNC(vkCreateImageView);
    LOAD_DEVICE_FUNC(vkDestroyImageView);
    LOAD_DEVICE_FUNC(vkCreateImage);
    LOAD_DEVICE_FUNC(vkDestroyImage);
    LOAD_DEVICE_FUNC(vkCreateBuffer);
    LOAD_DEVICE_FUNC(vkDestroyBuffer);
    LOAD_DEVICE_FUNC(vkAllocateMemory);
    LOAD_DEVICE_FUNC(vkFreeMemory);
    LOAD_DEVICE_FUNC(vkBindBufferMemory);
    LOAD_DEVICE_FUNC(vkBindImageMemory);
    LOAD_DEVICE_FUNC(vkMapMemory);
    LOAD_DEVICE_FUNC(vkUnmapMemory);
    LOAD_DEVICE_FUNC(vkGetImageMemoryRequirements);
    LOAD_DEVICE_FUNC(vkGetBufferMemoryRequirements);
    LOAD_DEVICE_FUNC(vkCmdBeginRendering);
    LOAD_DEVICE_FUNC(vkCmdEndRendering);
    LOAD_DEVICE_FUNC(vkCmdBindPipeline);
    LOAD_DEVICE_FUNC(vkCmdSetViewport);
    LOAD_DEVICE_FUNC(vkCmdSetScissor);
    LOAD_DEVICE_FUNC(vkCmdDraw);
    LOAD_DEVICE_FUNC(vkCmdBindVertexBuffers);
    LOAD_DEVICE_FUNC(vkCmdCopyBuffer);
    LOAD_DEVICE_FUNC(vkCmdPipelineBarrier);
    LOAD_DEVICE_FUNC(vkCmdPushConstants);
    LOAD_DEVICE_FUNC(vkResetCommandBuffer);
    
    // Swapchain
    LOAD_DEVICE_FUNC(vkCreateSwapchainKHR);
    LOAD_DEVICE_FUNC(vkDestroySwapchainKHR);
    LOAD_DEVICE_FUNC(vkGetSwapchainImagesKHR);
    LOAD_DEVICE_FUNC(vkAcquireNextImageKHR);
    LOAD_DEVICE_FUNC(vkQueuePresentKHR);

#undef LOAD_DEVICE_FUNC
}

#endif // VOLK_IMPLEMENTATION

#endif // VOLK_H_
