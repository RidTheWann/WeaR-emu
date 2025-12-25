#pragma once

/**
 * @file WeaR_EmulatorCore.h
 * @brief Central Emulator Core - Ties all subsystems together
 * 
 * Manages the Run/Pause/Stop state machine and coordinates initialization order.
 */

#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <functional>
#include <cstdint>

namespace WeaR {

// Forward declarations
class WeaR_Memory;
class WeaR_Cpu;
class WeaR_RenderEngine;
class WeaR_Specs;
struct WeaR_Context;

// =============================================================================
// EMULATOR STATE
// =============================================================================

enum class EmuState {
    Idle,       // No game loaded, waiting
    Booting,    // Loading game, initializing subsystems
    Running,    // Game is executing
    Paused,     // Game paused, can resume
    Stopping,   // Shutting down, cleaning up
    Error       // Fatal error occurred
};

/**
 * @brief Get state name for display
 */
[[nodiscard]] const char* getEmuStateName(EmuState state);

// =============================================================================
// EMULATOR CORE CLASS
// =============================================================================

/**
 * @brief Central emulator orchestrator
 */
class WeaR_EmulatorCore {
public:
    using StateCallback = std::function<void(EmuState newState)>;
    using LogCallback = std::function<void(const std::string& message)>;

    WeaR_EmulatorCore();
    ~WeaR_EmulatorCore();

    // Non-copyable
    WeaR_EmulatorCore(const WeaR_EmulatorCore&) = delete;
    WeaR_EmulatorCore& operator=(const WeaR_EmulatorCore&) = delete;

    // =========================================================================
    // INITIALIZATION
    // =========================================================================

    /**
     * @brief Initialize core with hardware specs
     * @param specs Detected hardware capabilities
     * @return true if successful
     */
    bool initialize(const WeaR_Specs& specs);

    /**
     * @brief Shutdown all subsystems
     */
    void shutdown();

    /**
     * @brief Check if initialized
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    // =========================================================================
    // GAME LOADING
    // =========================================================================

    /**
     * @brief Load a game from file
     * @param path Path to ELF/SELF file
     * @return Entry point address, or 0 on failure
     */
    [[nodiscard]] uint64_t loadGame(const std::string& path);

    /**
     * @brief Load internal BIOS for testing
     * @return Entry point address
     */
    [[nodiscard]] uint64_t loadInternalBios();

    /**
     * @brief Check if game is loaded
     */
    [[nodiscard]] bool isGameLoaded() const { return m_gameLoaded; }

    /**
     * @brief Get loaded game path
     */
    [[nodiscard]] const std::string& getGamePath() const { return m_gamePath; }

    // =========================================================================
    // STATE CONTROL
    // =========================================================================

    /**
     * @brief Start/Resume emulation
     * @return true if state changed
     */
    bool run();

    /**
     * @brief Pause emulation
     * @return true if state changed
     */
    bool pause();

    /**
     * @brief Stop emulation and reset
     * @return true if state changed
     */
    bool stop();

    /**
     * @brief Toggle pause state
     */
    bool togglePause();

    /**
     * @brief Get current state
     */
    [[nodiscard]] EmuState getState() const { return m_state.load(); }

    /**
     * @brief Check if running
     */
    [[nodiscard]] bool isRunning() const { return m_state == EmuState::Running; }

    /**
     * @brief Check if paused
     */
    [[nodiscard]] bool isPaused() const { return m_state == EmuState::Paused; }

    // =========================================================================
    // CALLBACKS
    // =========================================================================

    /**
     * @brief Set state change callback
     */
    void setStateCallback(StateCallback callback);

    /**
     * @brief Set log callback
     */
    void setLogCallback(LogCallback callback);

    // =========================================================================
    // SUBSYSTEM ACCESS
    // =========================================================================

    [[nodiscard]] WeaR_Memory* getMemory() const { return m_memory.get(); }
    [[nodiscard]] WeaR_Cpu* getCpu() const { return m_cpu.get(); }
    [[nodiscard]] WeaR_RenderEngine* getRenderer() const { return m_renderer; }

    /**
     * @brief Get CPU context snapshot
     */
    [[nodiscard]] WeaR_Context getCpuSnapshot() const;

    /**
     * @brief Get instruction count
     */
    [[nodiscard]] uint64_t getInstructionCount() const;

    // =========================================================================
    // RENDERER INTEGRATION
    // =========================================================================

    /**
     * @brief Set external render engine reference
     */
    void setRenderer(WeaR_RenderEngine* renderer);

private:
    void log(const std::string& message);
    void setState(EmuState newState);
    void cpuThreadMain();
    void initializeHLE();

    // State
    std::atomic<EmuState> m_state{EmuState::Idle};
    bool m_initialized = false;
    bool m_gameLoaded = false;
    std::string m_gamePath;
    uint64_t m_entryPoint = 0;

    // Subsystems
    std::unique_ptr<WeaR_Memory> m_memory;
    std::unique_ptr<WeaR_Cpu> m_cpu;
    WeaR_RenderEngine* m_renderer = nullptr;

    // CPU Thread
    std::thread m_cpuThread;
    std::atomic<bool> m_cpuRunning{false};

    // Callbacks
    StateCallback m_stateCallback;
    LogCallback m_logCallback;
    mutable std::mutex m_callbackMutex;
};

/**
 * @brief Get global emulator core instance
 */
WeaR_EmulatorCore& getEmulatorCore();

} // namespace WeaR
