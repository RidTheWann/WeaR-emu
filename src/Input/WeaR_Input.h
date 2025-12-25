#pragma once

/**
 * @file WeaR_Input.h
 * @brief PS4 Controller Emulation via Keyboard/Mouse
 * 
 * Maps host input to PS4 ScePadData format for libpad HLE.
 */

#include <cstdint>
#include <atomic>
#include <mutex>

namespace WeaR {

// =============================================================================
// PS4/SCE PAD BUTTON BITMASKS
// =============================================================================

namespace PadButton {
    constexpr uint32_t L3         = 0x0002;
    constexpr uint32_t R3         = 0x0004;
    constexpr uint32_t OPTIONS    = 0x0008;
    constexpr uint32_t UP         = 0x0010;
    constexpr uint32_t RIGHT      = 0x0020;
    constexpr uint32_t DOWN       = 0x0040;
    constexpr uint32_t LEFT       = 0x0080;
    constexpr uint32_t L2         = 0x0100;
    constexpr uint32_t R2         = 0x0200;
    constexpr uint32_t L1         = 0x0400;
    constexpr uint32_t R1         = 0x0800;
    constexpr uint32_t TRIANGLE   = 0x1000;
    constexpr uint32_t CIRCLE     = 0x2000;
    constexpr uint32_t CROSS      = 0x4000;
    constexpr uint32_t SQUARE     = 0x8000;
    constexpr uint32_t TOUCHPAD   = 0x00100000;
    constexpr uint32_t SHARE      = 0x00000001;
}

// SCE Button aliases for XInput mapping
constexpr uint32_t SCE_PAD_BUTTON_CROSS     = PadButton::CROSS;
constexpr uint32_t SCE_PAD_BUTTON_CIRCLE    = PadButton::CIRCLE;
constexpr uint32_t SCE_PAD_BUTTON_SQUARE    = PadButton::SQUARE;
constexpr uint32_t SCE_PAD_BUTTON_TRIANGLE  = PadButton::TRIANGLE;
constexpr uint32_t SCE_PAD_BUTTON_L1        = PadButton::L1;
constexpr uint32_t SCE_PAD_BUTTON_R1        = PadButton::R1;
constexpr uint32_t SCE_PAD_BUTTON_L2        = PadButton::L2;
constexpr uint32_t SCE_PAD_BUTTON_R2        = PadButton::R2;
constexpr uint32_t SCE_PAD_BUTTON_L3        = PadButton::L3;
constexpr uint32_t SCE_PAD_BUTTON_R3        = PadButton::R3;
constexpr uint32_t SCE_PAD_BUTTON_OPTIONS   = PadButton::OPTIONS;
constexpr uint32_t SCE_PAD_BUTTON_TOUCH_PAD = PadButton::TOUCHPAD;
constexpr uint32_t SCE_PAD_BUTTON_UP        = PadButton::UP;
constexpr uint32_t SCE_PAD_BUTTON_DOWN      = PadButton::DOWN;
constexpr uint32_t SCE_PAD_BUTTON_LEFT      = PadButton::LEFT;
constexpr uint32_t SCE_PAD_BUTTON_RIGHT     = PadButton::RIGHT;

// =============================================================================
// CONTROLLER STATE STRUCTURE
// =============================================================================

/**
 * @brief Current controller state (matches ScePadData layout)
 */
struct WeaR_ControllerState {
    uint32_t buttons = 0;           // Button bitmask
    
    // Left analog stick (0-255, center = 128)
    uint8_t leftStickX = 128;
    uint8_t leftStickY = 128;
    
    // Right analog stick (0-255, center = 128)
    uint8_t rightStickX = 128;
    uint8_t rightStickY = 128;
    
    // Analog triggers (0-255)
    uint8_t l2Analog = 0;
    uint8_t r2Analog = 0;
    
    // Touch pad (simplified)
    uint16_t touchX = 960;   // Center of 1920 touchpad
    uint16_t touchY = 470;   // Center of 942 touchpad
    bool touchActive = false;
    
    // Motion (placeholder)
    float accelerometerX = 0;
    float accelerometerY = -1.0f;  // Gravity
    float accelerometerZ = 0;
    float gyroX = 0, gyroY = 0, gyroZ = 0;

    void reset() {
        buttons = 0;
        leftStickX = leftStickY = 128;
        rightStickX = rightStickY = 128;
        l2Analog = r2Analog = 0;
        touchActive = false;
    }
};

// =============================================================================
// KEYBOARD MAPPING
// =============================================================================

/**
 * @brief Keyboard to PS4 button mapping entry
 */
struct KeyMapping {
    int qtKey;              // Qt::Key_* value
    uint32_t padButton;     // PadButton::* value
    bool isAnalog;          // Affects analog stick instead
    int analogAxis;         // 0=LX, 1=LY, 2=RX, 3=RY
    int analogValue;        // Value when pressed (0 or 255)
};

// =============================================================================
// INPUT MANAGER CLASS
// =============================================================================

/**
 * @brief Thread-safe input manager singleton
 */
class WeaR_InputManager {
public:
    /**
     * @brief Get singleton instance
     */
    static WeaR_InputManager& get();

    /**
     * @brief Handle keyboard key state change
     * @param qtKey Qt::Key_* value
     * @param pressed True if pressed, false if released
     */
    void handleKeyPress(int qtKey, bool pressed);

    /**
     * @brief Handle mouse button state change
     */
    void handleMouseButton(int button, bool pressed);

    /**
     * @brief Handle mouse movement
     */
    void handleMouseMove(int deltaX, int deltaY);

    /**
     * @brief Get current controller state (thread-safe copy)
     */
    [[nodiscard]] WeaR_ControllerState getPadState() const;

    /**
     * @brief Check if any input is active
     */
    [[nodiscard]] bool hasInput() const;

    /**
     * @brief Reset all input to default state
     */
    void reset();

    /**
     * @brief Set custom key mapping
     */
    void setKeyMapping(int qtKey, uint32_t padButton);

    /**
     * @brief Enable/disable mouse look for right stick
     */
    void setMouseLookEnabled(bool enabled) { m_mouseLookEnabled = enabled; }
    [[nodiscard]] bool isMouseLookEnabled() const { return m_mouseLookEnabled; }

    /**
     * @brief Get statistics
     */
    [[nodiscard]] uint64_t getInputEventCount() const { return m_inputEventCount; }

private:
    WeaR_InputManager();
    ~WeaR_InputManager() = default;

    void setupDefaultMappings();
    void applyDigitalToAnalog();

    WeaR_ControllerState m_state;
    mutable std::mutex m_mutex;
    
    // Key state tracking for analog simulation
    bool m_keyStates[512] = {};  // Qt key states
    bool m_mouseLookEnabled = false;
    
    std::atomic<uint64_t> m_inputEventCount{0};
};

// =============================================================================
// SCE PAD DATA STRUCTURE (for memory writing)
// =============================================================================

/**
 * @brief PS4 ScePadData structure layout for HLE
 */
#pragma pack(push, 1)
struct ScePadData {
    uint32_t buttons;           // 0x00 (4)
    uint8_t  leftStickX;        // 0x04 (1)
    uint8_t  leftStickY;        // 0x05 (1)
    uint8_t  rightStickX;       // 0x06 (1)
    uint8_t  rightStickY;       // 0x07 (1)
    uint8_t  analogL2;          // 0x08 (1)
    uint8_t  analogR2;          // 0x09 (1)
    uint16_t padding1;          // 0x0A (2)
    float    orientationX;      // 0x0C (4)
    float    orientationY;      // 0x10 (4)
    float    orientationZ;      // 0x14 (4)
    float    orientationW;      // 0x18 (4)
    float    accelerometerX;    // 0x1C (4)
    float    accelerometerY;    // 0x20 (4)
    float    accelerometerZ;    // 0x24 (4)
    float    gyroX;             // 0x28 (4)
    float    gyroY;             // 0x2C (4)
    float    gyroZ;             // 0x30 (4)
    uint8_t  touchData[24];     // 0x34 (24) -> end at 0x4C
    uint8_t  connected;         // 0x4C (1)
    uint8_t  padding3[3];       // 0x4D (3) -> align timestamp to 0x50
    uint64_t timestamp;         // 0x50 (8)
    uint8_t  extensionData[12]; // 0x58 (12)
    uint8_t  connectedCount;    // 0x64 (1)
    uint8_t  padding2[3];       // 0x65 (3) -> total = 0x68
};
#pragma pack(pop)

static_assert(sizeof(ScePadData) == 0x68, "ScePadData size mismatch");

// =============================================================================
// XINPUT-BASED INPUT CLASS
// =============================================================================

/**
 * @brief XInput/Keyboard hybrid input handler
 */
class WeaR_Input {
public:
    WeaR_Input();
    ~WeaR_Input();
    WeaR_Input(WeaR_Input&&) noexcept;
    WeaR_Input& operator=(WeaR_Input&&) noexcept;

    // Delete copy
    WeaR_Input(const WeaR_Input&) = delete;
    WeaR_Input& operator=(const WeaR_Input&) = delete;

    /**
     * @brief Poll current controller state
     */
    [[nodiscard]] ScePadData poll();

    /**
     * @brief Set keyboard fallback state
     */
    void setKeyboardState(const ScePadData& state);

    /**
     * @brief Check if XInput controller is connected
     */
    [[nodiscard]] bool isControllerConnected() const;

    /**
     * @brief Re-detect connected controllers
     */
    void detectControllers();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace WeaR
