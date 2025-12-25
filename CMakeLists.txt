cmake_minimum_required(VERSION 3.28)
project(WeaR-emu VERSION 0.1.0 LANGUAGES C CXX)

# ============================================================================
# C++ Standard Configuration (MSVC 2026 / C++23)
# ============================================================================
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC-specific flags for latest standard support
if(MSVC)
    add_compile_options(/std:c++latest /W4 /permissive- /Zc:__cplusplus)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN VK_NO_PROTOTYPES)
endif()

# ============================================================================
# Output Directories
# ============================================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ============================================================================
# Qt6 Configuration
# ============================================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Gui 
    Widgets 
    Multimedia
)

# ============================================================================
# Vulkan SDK Configuration
# ============================================================================
find_package(Vulkan REQUIRED)

# Volk (Vulkan meta-loader) - Header-only
set(VOLK_DIR "${CMAKE_SOURCE_DIR}/external/volk")
if(EXISTS "${VOLK_DIR}/volk.h")
    add_library(volk INTERFACE)
    target_include_directories(volk INTERFACE ${VOLK_DIR})
    target_compile_definitions(volk INTERFACE VK_NO_PROTOTYPES)
else()
    message(STATUS "Volk not found at ${VOLK_DIR}, using Vulkan SDK loader directly")
    add_library(volk INTERFACE)
    target_link_libraries(volk INTERFACE Vulkan::Vulkan)
endif()

# VMA (Vulkan Memory Allocator) - Header-only
set(VMA_DIR "${CMAKE_SOURCE_DIR}/external/VulkanMemoryAllocator/include")
if(EXISTS "${VMA_DIR}/vk_mem_alloc.h")
    add_library(VMA INTERFACE)
    target_include_directories(VMA INTERFACE ${VMA_DIR})
else()
    message(WARNING "VMA not found at ${VMA_DIR}. Please add VulkanMemoryAllocator.")
    add_library(VMA INTERFACE)
endif()

# ============================================================================
# Source Files
# ============================================================================
set(WEAR_SOURCES
    src/main.cpp
    
    # Hardware
    src/Hardware/HardwareDetector.cpp
    
    # Core
    src/Core/WeaR_Memory.cpp
    src/Core/WeaR_Cpu.cpp
    src/Core/WeaR_System.cpp
    src/Core/WeaR_EmulatorCore.cpp
    
    # Loader
    src/Loader/WeaR_ElfLoader.cpp
    
    # Graphics
    src/Graphics/WeaR_RenderEngine.cpp
    src/Graphics/WeaR_RenderQueue.cpp
    src/Graphics/WeaR_ShaderManager.cpp
    
    # GUI
    src/GUI/WeaR_GUI.cpp
    src/GUI/WeaR_Logger.cpp
    
    # HLE
    src/HLE/WeaR_Syscalls.cpp
    src/HLE/Graphics/WeaR_GnmDriver.cpp
    src/HLE/FileSystem/WeaR_VFS.cpp
    src/HLE/Modules/WeaR_LibPad.cpp
    src/HLE/Modules/WeaR_LibFS.cpp
    src/HLE/Modules/WeaR_LibAudio.cpp
    
    # Input
    src/Input/WeaR_Input.cpp
    
    # Audio
    src/Audio/WeaR_AudioManager.cpp
    
    # External Libraries (compiled sources)
    external/volk/volk.c
    external/VulkanMemoryAllocator/vma.cpp
)

set(WEAR_HEADERS
    src/Hardware/HardwareDetector.h
    
    src/Core/WeaR_Memory.h
    src/Core/WeaR_Cpu.h
    src/Core/WeaR_System.h
    src/Core/WeaR_EmulatorCore.h
    src/Core/WeaR_InternalBios.h
    
    src/Loader/WeaR_ElfLoader.h
    
    src/Graphics/WeaR_RenderEngine.h
    src/Graphics/WeaR_RenderQueue.h
    src/Graphics/WeaR_ShaderManager.h
    
    src/GUI/WeaR_GUI.h
    src/GUI/WeaR_Logger.h
    
    src/HLE/WeaR_Syscalls.h
    src/HLE/Graphics/WeaR_GnmDriver.h
    src/HLE/Graphics/PM4_Packets.h
    src/HLE/FileSystem/WeaR_VFS.h
    
    src/Input/WeaR_Input.h
    
    src/Audio/WeaR_AudioManager.h
)

# ============================================================================
# Main Executable
# ============================================================================
add_executable(WeaR-emu WIN32
    ${WEAR_SOURCES}
    ${WEAR_HEADERS}
    resources/resources.qrc
)

target_include_directories(WeaR-emu PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(WeaR-emu PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    volk
    VMA
    dwmapi
    uxtheme
)

# ============================================================================
# Post-Build: Copy Qt DLLs (Windows)
# ============================================================================
if(WIN32)
    add_custom_command(TARGET WeaR-emu POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Core>
            $<TARGET_FILE:Qt6::Gui>
            $<TARGET_FILE:Qt6::Widgets>
            $<TARGET_FILE:Qt6::Multimedia>
            $<TARGET_FILE_DIR:WeaR-emu>
    )
endif()

# ============================================================================
# Installation
# ============================================================================
install(TARGETS WeaR-emu
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY resources/styles/ DESTINATION bin/styles)
