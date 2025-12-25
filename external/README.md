# External Dependencies

This directory contains external libraries used by WeaR-emu.

## Required Libraries

### Volk (Vulkan Meta-Loader)
```bash
git clone https://github.com/zeux/volk.git external/volk
```

### VulkanMemoryAllocator (VMA)
```bash
git clone https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git external/VulkanMemoryAllocator
```

## Directory Structure After Setup
```
external/
├── volk/
│   └── volk.h
└── VulkanMemoryAllocator/
    └── include/
        └── vk_mem_alloc.h
```

## Alternative: CMake FetchContent
The CMakeLists.txt can be modified to use FetchContent for automatic dependency fetching:

```cmake
include(FetchContent)

FetchContent_Declare(
    volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG master
)

FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG master
)

FetchContent_MakeAvailable(volk VulkanMemoryAllocator)
```
