#include "HLE/WeaR_Syscalls.h"
#include "Input/WeaR_Input.h"
#include "GUI/WeaR_Logger.h"

#include <iostream>
#include <format>
#include <cstring>
#include <chrono>

/**
 * @file WeaR_LibPad.cpp
 * @brief PS4 libpad HLE implementation
 * 
 * Handles scePadReadState and related controller syscalls.
 */

namespace WeaR {
namespace LibPad {

// =============================================================================
// SYSCALL NUMBERS
// =============================================================================

namespace Syscall {
    constexpr uint64_t SYS_scePadRead           = 570;
    constexpr uint64_t SYS_scePadReadState      = 571;
    constexpr uint64_t SYS_scePadOpen           = 572;
    constexpr uint64_t SYS_scePadClose          = 573;
    constexpr uint64_t SYS_scePadGetHandle      = 574;
    constexpr uint64_t SYS_scePadSetVibration   = 575;
}

// =============================================================================
// HELPER FUNCTIONS
// =============================================================================

/**
 * @brief Write ScePadData to game memory
 */
void writePadDataToMemory(WeaR_Memory& mem, uint64_t outputPtr, const WeaR_ControllerState& state) {
    ScePadData padData{};
    
    // Fill structure
    padData.buttons = state.buttons;
    padData.leftStickX = state.leftStickX;
    padData.leftStickY = state.leftStickY;
    padData.rightStickX = state.rightStickX;
    padData.rightStickY = state.rightStickY;
    padData.analogL2 = state.l2Analog;
    padData.analogR2 = state.r2Analog;
    
    // Orientation quaternion (identity)
    padData.orientationX = 0.0f;
    padData.orientationY = 0.0f;
    padData.orientationZ = 0.0f;
    padData.orientationW = 1.0f;
    
    // Accelerometer (gravity pointing down)
    padData.accelerometerX = state.accelerometerX;
    padData.accelerometerY = state.accelerometerY;
    padData.accelerometerZ = state.accelerometerZ;
    
    // Gyroscope
    padData.gyroX = state.gyroX;
    padData.gyroY = state.gyroY;
    padData.gyroZ = state.gyroZ;
    
    // Touch data (simplified)
    std::memset(padData.touchData, 0, sizeof(padData.touchData));
    
    // Connection status
    padData.connected = 1;  // Always connected
    padData.connectedCount = 1;
    
    // Timestamp
    auto now = std::chrono::steady_clock::now();
    padData.timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        now.time_since_epoch()).count();
    
    // Write to memory byte by byte (safe method)
    const uint8_t* dataBytes = reinterpret_cast<const uint8_t*>(&padData);
    for (size_t i = 0; i < sizeof(ScePadData); ++i) {
        mem.write<uint8_t>(outputPtr + i, dataBytes[i]);
    }
}

// =============================================================================
// HLE HANDLERS
// =============================================================================

/**
 * @brief scePadReadState - Read controller state
 * 
 * int scePadReadState(int handle, ScePadData* data)
 * RDI = handle (controller number, usually 0)
 * RSI = output pointer to ScePadData
 */
SyscallResult hle_scePadReadState(
    WeaR_Context& ctx, 
    WeaR_Memory& mem,
    uint64_t handle, 
    uint64_t outputPtr,
    uint64_t, uint64_t, uint64_t, uint64_t)
{
    (void)ctx;
    (void)handle;  // Ignore handle for now (single controller)
    
    if (outputPtr == 0) {
        return SyscallResult{-1, false, "scePadReadState: null output pointer"};
    }
    
    // Get current input state
    WeaR_ControllerState state = WeaR_InputManager::get().getPadState();
    
    // Write to game memory
    try {
        writePadDataToMemory(mem, outputPtr, state);
    } catch (const std::exception& e) {
        return SyscallResult{-14, false, std::format("scePadReadState: {}", e.what())};
    }
    
    return SyscallResult{0, true, ""};  // Success
}

/**
 * @brief scePadOpen - Open controller handle
 * 
 * int scePadOpen(int userID, int type, int index, void* param)
 */
SyscallResult hle_scePadOpen(
    WeaR_Context&, WeaR_Memory&,
    uint64_t userID, uint64_t type, uint64_t index,
    uint64_t, uint64_t, uint64_t)
{
    (void)userID;
    (void)type;
    (void)index;
    
    // Return handle 0 (single controller)
    getLogger().log(std::format("scePadOpen(user={}, type={}, index={})", 
                                 userID, type, index), LogLevel::Debug);
    return SyscallResult{0, true, ""};
}

/**
 * @brief scePadClose - Close controller handle
 */
SyscallResult hle_scePadClose(
    WeaR_Context&, WeaR_Memory&,
    uint64_t handle, uint64_t, uint64_t,
    uint64_t, uint64_t, uint64_t)
{
    (void)handle;
    return SyscallResult{0, true, ""};
}

/**
 * @brief scePadSetVibration - Set controller vibration
 */
SyscallResult hle_scePadSetVibration(
    WeaR_Context&, WeaR_Memory&,
    uint64_t handle, uint64_t leftMotor, uint64_t rightMotor,
    uint64_t, uint64_t, uint64_t)
{
    (void)handle;
    
    // Log vibration request (could trigger Windows haptics/gamepad rumble)
    if (leftMotor > 0 || rightMotor > 0) {
        getLogger().log(std::format("scePadSetVibration: L={}, R={}", 
                                     leftMotor, rightMotor), LogLevel::Debug);
    }
    
    return SyscallResult{0, true, ""};
}

// =============================================================================
// REGISTRATION
// =============================================================================

/**
 * @brief Register libpad syscall handlers with dispatcher
 */
void registerLibPadHandlers(WeaR_Syscalls& dispatcher) {
    dispatcher.registerHandler(Syscall::SYS_scePadReadState, hle_scePadReadState);
    dispatcher.registerHandler(Syscall::SYS_scePadRead, hle_scePadReadState);  // Alias
    dispatcher.registerHandler(Syscall::SYS_scePadOpen, hle_scePadOpen);
    dispatcher.registerHandler(Syscall::SYS_scePadClose, hle_scePadClose);
    dispatcher.registerHandler(Syscall::SYS_scePadSetVibration, hle_scePadSetVibration);
    
    std::cout << "[HLE] libpad handlers registered\n";
}

} // namespace LibPad
} // namespace WeaR
