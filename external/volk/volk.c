/**
 * @file volk.c
 * @brief Volk implementation file
 * 
 * This file triggers the compilation of the volk implementation
 * by defining VOLK_IMPLEMENTATION before including the header.
 */

#if defined(_WIN32)
    #define VK_USE_PLATFORM_WIN32_KHR
#endif

#define VOLK_IMPLEMENTATION
#include "volk.h"
