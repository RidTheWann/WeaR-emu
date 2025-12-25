#include "WeaR_Input.h"

#include <QKeySequence>
#include <Qt>
#include <iostream>
#include <format>
#include <cstring>

namespace WeaR {

// =============================================================================
// SINGLETON INSTANCE
// =============================================================================

WeaR_InputManager& WeaR_InputManager::get() {
    static WeaR_InputManager instance;
    return instance;
}

// =============================================================================
// CONSTRUCTOR
// =============================================================================

WeaR_InputManager::WeaR_InputManager() {
    std::memset(m_keyStates, 0, sizeof(m_keyStates));
    setupDefaultMappings();
    std::cout << "[Input] InputManager initialized\n";
}

// =============================================================================
// DEFAULT KEY MAPPINGS
// =============================================================================

void WeaR_InputManager::setupDefaultMappings() {
    // Default mappings are handled directly in handleKeyPress
    // This could be extended to load from config file
}

// =============================================================================
// KEY HANDLING
// =============================================================================

void WeaR_InputManager::handleKeyPress(int qtKey, bool pressed) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_inputEventCount++;

    // Track key state for analog simulation
    if (qtKey >= 0 && qtKey < 512) {
        m_keyStates[qtKey] = pressed;
    }

    // === BUTTON MAPPINGS ===
    uint32_t button = 0;
    
    // Action buttons (IJKL cluster - right hand)
    if (qtKey == Qt::Key_K)       button = PadButton::CROSS;      // K = X (Confirm)
    else if (qtKey == Qt::Key_L)  button = PadButton::CIRCLE;     // L = O (Cancel)
    else if (qtKey == Qt::Key_J)  button = PadButton::SQUARE;     // J = □
    else if (qtKey == Qt::Key_I)  button = PadButton::TRIANGLE;   // I = △
    
    // D-Pad (Arrow keys)
    else if (qtKey == Qt::Key_Up)    button = PadButton::UP;
    else if (qtKey == Qt::Key_Down)  button = PadButton::DOWN;
    else if (qtKey == Qt::Key_Left)  button = PadButton::LEFT;
    else if (qtKey == Qt::Key_Right) button = PadButton::RIGHT;
    
    // Shoulder buttons
    else if (qtKey == Qt::Key_Q)  button = PadButton::L1;
    else if (qtKey == Qt::Key_E)  button = PadButton::R1;
    else if (qtKey == Qt::Key_1)  button = PadButton::L2;
    else if (qtKey == Qt::Key_3)  button = PadButton::R2;
    
    // System buttons
    else if (qtKey == Qt::Key_Return || qtKey == Qt::Key_Enter) 
        button = PadButton::OPTIONS;
    else if (qtKey == Qt::Key_Backspace) 
        button = PadButton::SHARE;
    else if (qtKey == Qt::Key_T)  button = PadButton::TOUCHPAD;
    
    // Stick buttons
    else if (qtKey == Qt::Key_F)  button = PadButton::L3;
    else if (qtKey == Qt::Key_G)  button = PadButton::R3;

    // Apply button state
    if (button != 0) {
        if (pressed) {
            m_state.buttons |= button;
        } else {
            m_state.buttons &= ~button;
        }
    }

    // === ANALOG STICK SIMULATION (WASD = Left Stick) ===
    // Check current WASD state
    bool w = (qtKey == Qt::Key_W) ? pressed : m_keyStates[Qt::Key_W & 0x1FF];
    bool a = (qtKey == Qt::Key_A) ? pressed : m_keyStates[Qt::Key_A & 0x1FF];
    bool s = (qtKey == Qt::Key_S) ? pressed : m_keyStates[Qt::Key_S & 0x1FF];
    bool d = (qtKey == Qt::Key_D) ? pressed : m_keyStates[Qt::Key_D & 0x1FF];

    // Update key state for WASD
    if (qtKey == Qt::Key_W) m_keyStates[Qt::Key_W & 0x1FF] = pressed;
    if (qtKey == Qt::Key_A) m_keyStates[Qt::Key_A & 0x1FF] = pressed;
    if (qtKey == Qt::Key_S) m_keyStates[Qt::Key_S & 0x1FF] = pressed;
    if (qtKey == Qt::Key_D) m_keyStates[Qt::Key_D & 0x1FF] = pressed;

    // Calculate analog values (center = 128)
    if (w && !s) m_state.leftStickY = 0;       // Full up
    else if (s && !w) m_state.leftStickY = 255; // Full down
    else m_state.leftStickY = 128;              // Center

    if (a && !d) m_state.leftStickX = 0;       // Full left
    else if (d && !a) m_state.leftStickX = 255; // Full right
    else m_state.leftStickX = 128;              // Center

    // === ANALOG TRIGGERS ===
    if (qtKey == Qt::Key_1) m_state.l2Analog = pressed ? 255 : 0;
    if (qtKey == Qt::Key_3) m_state.r2Analog = pressed ? 255 : 0;
}

void WeaR_InputManager::handleMouseButton(int button, bool pressed) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_inputEventCount++;
    
    // Mouse buttons could map to additional controls
    // Left click = L2 aim, Right click = R2 shoot (common FPS layout)
    if (button == 1) {  // Left
        if (pressed) m_state.buttons |= PadButton::L2;
        else m_state.buttons &= ~PadButton::L2;
        m_state.l2Analog = pressed ? 255 : 0;
    } else if (button == 2) {  // Right
        if (pressed) m_state.buttons |= PadButton::R2;
        else m_state.buttons &= ~PadButton::R2;
        m_state.r2Analog = pressed ? 255 : 0;
    }
}

void WeaR_InputManager::handleMouseMove(int deltaX, int deltaY) {
    if (!m_mouseLookEnabled) return;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    m_inputEventCount++;
    
    // Map mouse movement to right stick
    // Scale and clamp to 0-255 range
    constexpr float sensitivity = 2.0f;
    
    int newX = 128 + static_cast<int>(deltaX * sensitivity);
    int newY = 128 + static_cast<int>(deltaY * sensitivity);
    
    m_state.rightStickX = static_cast<uint8_t>(std::clamp(newX, 0, 255));
    m_state.rightStickY = static_cast<uint8_t>(std::clamp(newY, 0, 255));
}

// =============================================================================
// STATE ACCESS
// =============================================================================

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
    std::memset(m_keyStates, 0, sizeof(m_keyStates));
}

void WeaR_InputManager::setKeyMapping(int qtKey, uint32_t padButton) {
    // Could store custom mappings in a map
    (void)qtKey;
    (void)padButton;
}

} // namespace WeaR
