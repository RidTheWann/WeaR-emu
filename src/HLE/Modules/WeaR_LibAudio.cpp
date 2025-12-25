#include "HLE/WeaR_Syscalls.h"
#include "Audio/WeaR_AudioManager.h"
#include "GUI/WeaR_Logger.h"

#include <iostream>
#include <format>
#include <cstring>

/**
 * @file WeaR_LibAudio.cpp
 * @brief PS4 Audio syscall HLE implementations
 * 
 * Handles sceAudioOut* syscalls for sound output.
 */

namespace WeaR {
namespace LibAudio {

// =============================================================================
// SYSCALL NUMBERS
// =============================================================================

namespace Syscall {
    constexpr uint64_t SYS_sceAudioOutInit        = 495;
    constexpr uint64_t SYS_sceAudioOutOpen        = 496;
    constexpr uint64_t SYS_sceAudioOutClose       = 497;
    constexpr uint64_t SYS_sceAudioOutOutput      = 498;
    constexpr uint64_t SYS_sceAudioOutOutputs     = 499;
    constexpr uint64_t SYS_sceAudioOutSetVolume   = 500;
    constexpr uint64_t SYS_sceAudioOutGetPortState = 501;
    constexpr uint64_t SYS_sceAudioOutGetSystemState = 502;
}

// =============================================================================
// HLE HANDLERS
// =============================================================================

/**
 * @brief sceAudioOutInit - Initialize audio subsystem
 * 
 * int32_t sceAudioOutInit(void)
 */
SyscallResult hle_sceAudioOutInit(
    WeaR_Context& ctx, 
    WeaR_Memory& mem,
    uint64_t, uint64_t, uint64_t,
    uint64_t, uint64_t, uint64_t)
{
    (void)ctx;
    (void)mem;
    
    bool success = WeaR_AudioManager::get().init();
    getLogger().log(std::string("sceAudioOutInit"), LogLevel::Debug);
    
    return SyscallResult{success ? 0 : -1, success, ""};
}

/**
 * @brief sceAudioOutOpen - Open audio output port
 * 
 * int32_t sceAudioOutOpen(int32_t userId, int32_t type, int32_t index, 
 *                         uint32_t len, uint32_t freq, uint32_t param)
 * RDI = userId
 * RSI = type
 * RDX = index  
 * R10 = len (sample count)
 * R8  = freq (sample rate)
 * R9  = param
 */
SyscallResult hle_sceAudioOutOpen(
    WeaR_Context& ctx, 
    WeaR_Memory& mem,
    uint64_t userId, 
    uint64_t type,
    uint64_t index, 
    uint64_t len,
    uint64_t freq, 
    uint64_t param)
{
    (void)ctx;
    (void)mem;
    (void)userId;
    
    int32_t handle = WeaR_AudioManager::get().openPort(
        static_cast<int32_t>(type),
        static_cast<int32_t>(index),
        static_cast<int32_t>(len),
        static_cast<int32_t>(freq),
        static_cast<uint32_t>(param)
    );
    
    getLogger().log(std::format("sceAudioOutOpen: type={}, len={}, freq={} -> handle={}",
                                 type, len, freq, handle), LogLevel::Debug);
    
    return SyscallResult{handle, handle >= 0, ""};
}

/**
 * @brief sceAudioOutClose - Close audio output port
 * 
 * int32_t sceAudioOutClose(int32_t handle)
 */
SyscallResult hle_sceAudioOutClose(
    WeaR_Context& ctx, 
    WeaR_Memory& mem,
    uint64_t handle, 
    uint64_t, uint64_t,
    uint64_t, uint64_t, uint64_t)
{
    (void)ctx;
    (void)mem;
    
    int32_t result = WeaR_AudioManager::get().closePort(static_cast<int32_t>(handle));
    return SyscallResult{result, result == 0, ""};
}

/**
 * @brief sceAudioOutOutput - Output audio samples (BLOCKING)
 * 
 * int32_t sceAudioOutOutput(int32_t handle, const void* ptr)
 * RDI = handle
 * RSI = ptr to PCM data
 * 
 * NOTE: This call blocks until the audio buffer is consumed.
 * We simulate this with sleep_for to prevent chipmunk effect.
 */
SyscallResult hle_sceAudioOutOutput(
    WeaR_Context& ctx, 
    WeaR_Memory& mem,
    uint64_t handle, 
    uint64_t ptr,
    uint64_t, uint64_t, uint64_t, uint64_t)
{
    (void)ctx;
    
    if (ptr == 0) {
        return SyscallResult{-1, false, "null pointer"};
    }
    
    // Get port to find sample count
    int32_t sampleCount = 256;  // Default
    WeaR_AudioManager::get().getPortParam(static_cast<int32_t>(handle), &sampleCount, nullptr);
    
    // Calculate data size: samples * channels * bytes_per_sample
    size_t dataSize = static_cast<size_t>(sampleCount) * AudioConstants::CHANNELS * AudioConstants::BYTES_PER_SAMPLE;
    
    // Read PCM data from game memory
    std::vector<uint8_t> pcmData(dataSize);
    try {
        for (size_t i = 0; i < dataSize; ++i) {
            pcmData[i] = mem.read<uint8_t>(ptr + i);
        }
    } catch (const std::exception& e) {
        return SyscallResult{-1, false, std::format("memory read failed: {}", e.what())};
    }
    
    // Output audio
    int32_t result = WeaR_AudioManager::get().output(
        static_cast<int32_t>(handle), 
        pcmData.data(), 
        dataSize
    );
    
    return SyscallResult{result, result == 0, ""};
}

/**
 * @brief sceAudioOutOutputs - Output to multiple ports at once
 */
SyscallResult hle_sceAudioOutOutputs(
    WeaR_Context& ctx, 
    WeaR_Memory& mem,
    uint64_t handle, 
    uint64_t ptr,
    uint64_t, uint64_t, uint64_t, uint64_t)
{
    // For simplicity, treat as single output
    return hle_sceAudioOutOutput(ctx, mem, handle, ptr, 0, 0, 0, 0);
}

/**
 * @brief sceAudioOutSetVolume - Set output volume
 * 
 * int32_t sceAudioOutSetVolume(int32_t handle, int32_t flag, int32_t* vol)
 */
SyscallResult hle_sceAudioOutSetVolume(
    WeaR_Context& ctx, 
    WeaR_Memory& mem,
    uint64_t handle, 
    uint64_t flag,
    uint64_t volPtr, 
    uint64_t, uint64_t, uint64_t)
{
    (void)ctx;
    (void)flag;
    
    float volume = 1.0f;
    if (volPtr != 0) {
        // Volume is typically 0-32767 in PS4
        int32_t vol = mem.read<int32_t>(volPtr);
        volume = static_cast<float>(vol) / 32767.0f;
    }
    
    int32_t result = WeaR_AudioManager::get().setVolume(static_cast<int32_t>(handle), volume);
    return SyscallResult{result, result == 0, ""};
}

/**
 * @brief sceAudioOutGetPortState - Get port status
 */
SyscallResult hle_sceAudioOutGetPortState(
    WeaR_Context& ctx, 
    WeaR_Memory& mem,
    uint64_t handle, 
    uint64_t statePtr,
    uint64_t, uint64_t, uint64_t, uint64_t)
{
    (void)ctx;
    (void)handle;
    
    if (statePtr != 0) {
        // Write a simple "active" state
        mem.write<uint32_t>(statePtr, 1);  // State = active
        mem.write<uint32_t>(statePtr + 4, 0);  // No error
    }
    
    return SyscallResult{0, true, ""};
}

/**
 * @brief sceAudioOutGetSystemState - Get overall audio system state
 */
SyscallResult hle_sceAudioOutGetSystemState(
    WeaR_Context& ctx, 
    WeaR_Memory& mem,
    uint64_t statePtr, 
    uint64_t, uint64_t,
    uint64_t, uint64_t, uint64_t)
{
    (void)ctx;
    
    if (statePtr != 0) {
        mem.write<uint32_t>(statePtr, 1);  // System ready
    }
    
    return SyscallResult{0, true, ""};
}

// =============================================================================
// REGISTRATION
// =============================================================================

/**
 * @brief Register audio syscall handlers with dispatcher
 */
void registerLibAudioHandlers(WeaR_Syscalls& dispatcher) {
    dispatcher.registerHandler(Syscall::SYS_sceAudioOutInit, hle_sceAudioOutInit);
    dispatcher.registerHandler(Syscall::SYS_sceAudioOutOpen, hle_sceAudioOutOpen);
    dispatcher.registerHandler(Syscall::SYS_sceAudioOutClose, hle_sceAudioOutClose);
    dispatcher.registerHandler(Syscall::SYS_sceAudioOutOutput, hle_sceAudioOutOutput);
    dispatcher.registerHandler(Syscall::SYS_sceAudioOutOutputs, hle_sceAudioOutOutputs);
    dispatcher.registerHandler(Syscall::SYS_sceAudioOutSetVolume, hle_sceAudioOutSetVolume);
    dispatcher.registerHandler(Syscall::SYS_sceAudioOutGetPortState, hle_sceAudioOutGetPortState);
    dispatcher.registerHandler(Syscall::SYS_sceAudioOutGetSystemState, hle_sceAudioOutGetSystemState);
    
    std::cout << "[HLE] libAudio handlers registered\n";
}

} // namespace LibAudio
} // namespace WeaR
