#include "WeaR_Input.h"
#include <format>
#include <iostream>
#include <cstring>
#include <memory>
#include <algorithm>

#ifdef _WIN32
#include <Windows.h>
#include <Xinput.h>
#pragma comment(lib, "xinput.lib")
#endif

namespace WeaR {

// ============================================================================
// XInput Button Mappings → PS4 DualShock 4
// ============================================================================
// Xbox A     → PS4 Cross (X)
// Xbox B     → PS4 Circle (O)
// Xbox X     → PS4 Square (□)
// Xbox Y     → PS4 Triangle (△)
// Xbox LB    → PS4 L1
// Xbox RB    → PS4 R1
// Xbox LT    → PS4 L2 (Trigger)
// Xbox RT    → PS4 R2 (Trigger)
// Xbox Start → PS4 Options
// Xbox Back  → PS4 Share/Touchpad Press
// Xbox LS    → PS4 L3
// Xbox RS    → PS4 R3

class WeaR_Input::Impl {
public:
    static constexpr SHORT DEADZONE_THRESHOLD = 8000;
    static constexpr BYTE TRIGGER_DEADZONE = 30;

    Impl() {
        detectControllers();
    }

    void detectControllers() {
        m_controllerConnected = false;
        m_controllerIndex = -1;

#ifdef _WIN32
        // Scan ALL ports 0-3 to find first connected controller
        for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i) {
            XINPUT_STATE state{};
            DWORD result = XInputGetState(i, &state);
            if (result == ERROR_SUCCESS) {
                m_controllerConnected = true;
                m_controllerIndex = static_cast<int>(i);
                std::cout << std::format("[INPUT] Controller found on Port {}\n", i);
                return;
            }
        }
#endif

        if (!m_controllerConnected) {
            std::cout << "[INPUT] No controller detected on ports 0-3\n";
        }
    }

    ScePadData poll() {
        ScePadData pad{};
        std::memset(&pad, 0, sizeof(pad));

#ifdef _WIN32
        // Re-scan for controller if not connected
        if (!m_controllerConnected || m_controllerIndex < 0) {
            detectControllers();
        }

        if (m_controllerConnected && m_controllerIndex >= 0) {
            XINPUT_STATE state{};
            DWORD result = XInputGetState(static_cast<DWORD>(m_controllerIndex), &state);
            
            if (result == ERROR_SUCCESS) {
                mapXInputToScePad(state, pad);
                pad.connected = 1;
            } else {
                // Controller disconnected - will re-scan on next poll
                std::cout << std::format("[INPUT] Controller on Port {} disconnected\n", m_controllerIndex);
                m_controllerConnected = false;
                m_controllerIndex = -1;
                pad.connected = 0;
            }
        } else {
            // Use keyboard fallback
            pad = m_keyboardState;
            pad.connected = 1;
        }
#else
        pad = m_keyboardState;
        pad.connected = 1;
#endif

        return pad;
    }

    void setKeyboardState(const ScePadData& state) {
        m_keyboardState = state;
    }

    bool isControllerConnected() const { return m_controllerConnected; }
    int getControllerPort() const { return m_controllerIndex; }

private:
#ifdef _WIN32
    // Apply deadzone to stick value
    SHORT applyDeadzone(SHORT value) const {
        if (std::abs(value) < DEADZONE_THRESHOLD) {
            return 0;
        }
        return value;
    }

    void mapXInputToScePad(const XINPUT_STATE& xinput, ScePadData& pad) {
        const XINPUT_GAMEPAD& gp = xinput.Gamepad;

        // Digital buttons
        pad.buttons = 0;
        
        if (gp.wButtons & XINPUT_GAMEPAD_A)             pad.buttons |= SCE_PAD_BUTTON_CROSS;
        if (gp.wButtons & XINPUT_GAMEPAD_B)             pad.buttons |= SCE_PAD_BUTTON_CIRCLE;
        if (gp.wButtons & XINPUT_GAMEPAD_X)             pad.buttons |= SCE_PAD_BUTTON_SQUARE;
        if (gp.wButtons & XINPUT_GAMEPAD_Y)             pad.buttons |= SCE_PAD_BUTTON_TRIANGLE;
        if (gp.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) pad.buttons |= SCE_PAD_BUTTON_L1;
        if (gp.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)pad.buttons |= SCE_PAD_BUTTON_R1;
        if (gp.wButtons & XINPUT_GAMEPAD_BACK)          pad.buttons |= SCE_PAD_BUTTON_TOUCH_PAD;
        if (gp.wButtons & XINPUT_GAMEPAD_START)         pad.buttons |= SCE_PAD_BUTTON_OPTIONS;
        if (gp.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)    pad.buttons |= SCE_PAD_BUTTON_L3;
        if (gp.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)   pad.buttons |= SCE_PAD_BUTTON_R3;
        if (gp.wButtons & XINPUT_GAMEPAD_DPAD_UP)       pad.buttons |= SCE_PAD_BUTTON_UP;
        if (gp.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)     pad.buttons |= SCE_PAD_BUTTON_DOWN;
        if (gp.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)     pad.buttons |= SCE_PAD_BUTTON_LEFT;
        if (gp.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)    pad.buttons |= SCE_PAD_BUTTON_RIGHT;

        // Triggers with deadzone (0-255 range)
        pad.analogL2 = (gp.bLeftTrigger > TRIGGER_DEADZONE) ? gp.bLeftTrigger : 0;
        pad.analogR2 = (gp.bRightTrigger > TRIGGER_DEADZONE) ? gp.bRightTrigger : 0;
        
        // Digital L2/R2 if trigger > threshold
        if (gp.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)  pad.buttons |= SCE_PAD_BUTTON_L2;
        if (gp.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) pad.buttons |= SCE_PAD_BUTTON_R2;

        // Left Stick with deadzone (convert from -32768..32767 to 0..255)
        SHORT lx = applyDeadzone(gp.sThumbLX);
        SHORT ly = applyDeadzone(gp.sThumbLY);
        pad.leftStickX = static_cast<uint8_t>((lx + 32768) >> 8);
        pad.leftStickY = static_cast<uint8_t>(255 - ((ly + 32768) >> 8)); // Invert Y

        // Right Stick with deadzone
        SHORT rx = applyDeadzone(gp.sThumbRX);
        SHORT ry = applyDeadzone(gp.sThumbRY);
        pad.rightStickX = static_cast<uint8_t>((rx + 32768) >> 8);
        pad.rightStickY = static_cast<uint8_t>(255 - ((ry + 32768) >> 8)); // Invert Y
    }
#endif

    bool m_controllerConnected = false;
    int m_controllerIndex = -1;
    ScePadData m_keyboardState{};
};

// ============================================================================
// Public Interface
// ============================================================================

WeaR_Input::WeaR_Input() : m_impl(std::make_unique<Impl>()) {}
WeaR_Input::~WeaR_Input() = default;
WeaR_Input::WeaR_Input(WeaR_Input&&) noexcept = default;
WeaR_Input& WeaR_Input::operator=(WeaR_Input&&) noexcept = default;

ScePadData WeaR_Input::poll() {
    return m_impl->poll();
}

void WeaR_Input::setKeyboardState(const ScePadData& state) {
    m_impl->setKeyboardState(state);
}

bool WeaR_Input::isControllerConnected() const {
    return m_impl->isControllerConnected();
}

void WeaR_Input::detectControllers() {
    m_impl->detectControllers();
}

// ============================================================================
// WeaR_InputManager Singleton Implementation
// ============================================================================

WeaR_InputManager& WeaR_InputManager::get() {
    static WeaR_InputManager instance;
    return instance;
}

WeaR_InputManager::WeaR_InputManager() {
    setupDefaultMappings();
}

void WeaR_InputManager::setupDefaultMappings() {
    // Default keyboard mappings are set up here
    // Qt::Key_W -> Left Stick Up, etc.
}

void WeaR_InputManager::handleKeyPress(int qtKey, bool pressed) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (qtKey >= 0 && qtKey < 512) {
        m_keyStates[qtKey] = pressed;
    }
    
    // Map common keys to pad buttons
    switch (qtKey) {
        case 0x01000012: // Qt::Key_Up
            if (pressed) m_state.buttons |= PadButton::UP;
            else m_state.buttons &= ~PadButton::UP;
            break;
        case 0x01000013: // Qt::Key_Down
            if (pressed) m_state.buttons |= PadButton::DOWN;
            else m_state.buttons &= ~PadButton::DOWN;
            break;
        case 0x01000014: // Qt::Key_Left
            if (pressed) m_state.buttons |= PadButton::LEFT;
            else m_state.buttons &= ~PadButton::LEFT;
            break;
        case 0x01000015: // Qt::Key_Right
            if (pressed) m_state.buttons |= PadButton::RIGHT;
            else m_state.buttons &= ~PadButton::RIGHT;
            break;
        case 0x5a: // Qt::Key_Z - Cross
            if (pressed) m_state.buttons |= PadButton::CROSS;
            else m_state.buttons &= ~PadButton::CROSS;
            break;
        case 0x58: // Qt::Key_X - Circle
            if (pressed) m_state.buttons |= PadButton::CIRCLE;
            else m_state.buttons &= ~PadButton::CIRCLE;
            break;
        case 0x43: // Qt::Key_C - Square
            if (pressed) m_state.buttons |= PadButton::SQUARE;
            else m_state.buttons &= ~PadButton::SQUARE;
            break;
        case 0x56: // Qt::Key_V - Triangle
            if (pressed) m_state.buttons |= PadButton::TRIANGLE;
            else m_state.buttons &= ~PadButton::TRIANGLE;
            break;
        case 0x01000004: // Qt::Key_Return - Options
            if (pressed) m_state.buttons |= PadButton::OPTIONS;
            else m_state.buttons &= ~PadButton::OPTIONS;
            break;
    }
    
    applyDigitalToAnalog();
    m_inputEventCount++;
}

void WeaR_InputManager::handleMouseButton(int button, bool pressed) {
    std::lock_guard<std::mutex> lock(m_mutex);
    // Button 1 = L2, Button 2 = R2 (example)
    if (button == 1) {
        m_state.l2Analog = pressed ? 255 : 0;
        if (pressed) m_state.buttons |= PadButton::L2;
        else m_state.buttons &= ~PadButton::L2;
    } else if (button == 2) {
        m_state.r2Analog = pressed ? 255 : 0;
        if (pressed) m_state.buttons |= PadButton::R2;
        else m_state.buttons &= ~PadButton::R2;
    }
    m_inputEventCount++;
}

void WeaR_InputManager::handleMouseMove(int deltaX, int deltaY) {
    if (!m_mouseLookEnabled) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Map mouse movement to right stick
    int newX = m_state.rightStickX + deltaX;
    int newY = m_state.rightStickY + deltaY;
    m_state.rightStickX = static_cast<uint8_t>(std::clamp(newX, 0, 255));
    m_state.rightStickY = static_cast<uint8_t>(std::clamp(newY, 0, 255));
    m_inputEventCount++;
}

WeaR_ControllerState WeaR_InputManager::getPadState() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state;
}

bool WeaR_InputManager::hasInput() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state.buttons != 0 || 
           m_state.leftStickX != 128 || m_state.leftStickY != 128 ||
           m_state.rightStickX != 128 || m_state.rightStickY != 128;
}

void WeaR_InputManager::reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_state.reset();
}

void WeaR_InputManager::setKeyMapping(int qtKey, uint32_t padButton) {
    // Custom key mapping - store in a map for advanced users
    (void)qtKey;
    (void)padButton;
}

void WeaR_InputManager::applyDigitalToAnalog() {
    // W/S keys for left stick Y
    if (m_keyStates[0x57]) m_state.leftStickY = 0;   // W = up
    else if (m_keyStates[0x53]) m_state.leftStickY = 255; // S = down
    else m_state.leftStickY = 128;
    
    // A/D keys for left stick X
    if (m_keyStates[0x41]) m_state.leftStickX = 0;   // A = left
    else if (m_keyStates[0x44]) m_state.leftStickX = 255; // D = right
    else m_state.leftStickX = 128;
}

} // namespace WeaR
