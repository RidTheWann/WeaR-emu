#pragma once

/**
 * @file WeaR_System.h
 * @brief System Orchestrator - Manages all emulator components
 * 
 * Coordinates:
 * - Memory subsystem
 * - CPU execution (threaded)
 * - Render engine
 * - ELF loading
 */

#include "Core/WeaR_Memory.h"
#include "Core/WeaR_Cpu.h"
#include "Loader/WeaR_ElfLoader.h"
#include "Hardware/HardwareDetector.h"

#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>
#include <string>

namespace WeaR {

// Forward declaration
class WeaR_RenderEngine;

// =============================================================================
// SYSTEM STATE
// =============================================================================

enum class SystemState : uint8_t {
    Uninitialized,
    Ready,         // All components initialized
    Running,       // CPU thread executing
    Paused,
    Stopped,
    Error
};

// =============================================================================
// SYSTEM CALLBACKS
// =============================================================================

struct SystemCallbacks {
    std::function<void(SystemState)> onStateChanged;
    std::function<void(const WeaR_Context&)> onCpuUpdate;
    std::function<void(const std::string&)> onLog;
    std::function<void(uint64_t)> onFrameComplete;
};

// =============================================================================
// WEAR_SYSTEM CLASS
// =============================================================================

/**
 * @brief Main system orchestrator
 */
class WeaR_System {
public:
    WeaR_System();
    ~WeaR_System();

    // Non-copyable
    WeaR_System(const WeaR_System&) = delete;
    WeaR_System& operator=(const WeaR_System&) = delete;

    // =========================================================================
    // Initialization
    // =========================================================================
    
    /**
     * @brief Initialize all subsystems
     * @param specs Hardware capabilities
     * @return true on success
     */
    bool initialize(const WeaR_Specs& specs);

    /**
     * @brief Load a game ELF
     * @param filepath Path to ELF file
     * @return Entry point address, or 0 on failure
     */
    uint64_t loadGame(const std::string& filepath);

    // =========================================================================
    // Execution Control
    // =========================================================================
    
    /**
     * @brief Boot the loaded game (starts CPU thread)
     */
    void boot();

    /**
     * @brief Stop execution
     */
    void stop();

    /**
     * @brief Pause execution
     */
    void pause();

    /**
     * @brief Resume from pause
     */
    void resume();

    /**
     * @brief Reset system to initial state
     */
    void reset();

    /**
     * @brief Shutdown all components
     */
    void shutdown();

    // =========================================================================
    // State Access
    // =========================================================================
    
    [[nodiscard]] SystemState getState() const { return m_state.load(); }
    [[nodiscard]] bool isRunning() const { return m_state.load() == SystemState::Running; }
    [[nodiscard]] bool isReady() const { return m_state.load() == SystemState::Ready; }

    [[nodiscard]] WeaR_Memory* getMemory() { return m_memory.get(); }
    [[nodiscard]] WeaR_Cpu* getCpu() { return m_cpu.get(); }
    [[nodiscard]] WeaR_RenderEngine* getRenderer() { return m_renderer; }

    /**
     * @brief Get current CPU context (thread-safe snapshot)
     */
    [[nodiscard]] WeaR_Context getCpuSnapshot() const;

    /**
     * @brief Get instruction count
     */
    [[nodiscard]] uint64_t getInstructionCount() const;

    // =========================================================================
    // Configuration
    // =========================================================================
    
    void setRenderer(WeaR_RenderEngine* renderer) { m_renderer = renderer; }
    void setCallbacks(const SystemCallbacks& callbacks) { m_callbacks = callbacks; }

private:
    void cpuThreadFunc();
    void setState(SystemState state);
    void handleSyscall(WeaR_Context& ctx);

    // Components
    std::unique_ptr<WeaR_Memory> m_memory;
    std::unique_ptr<WeaR_Cpu> m_cpu;
    std::unique_ptr<WeaR_ElfLoader> m_elfLoader;
    WeaR_RenderEngine* m_renderer = nullptr;  // Non-owning

    // Threading
    std::unique_ptr<std::thread> m_cpuThread;
    std::mutex m_cpuMutex;
    std::atomic<SystemState> m_state{SystemState::Uninitialized};

    // Game info
    uint64_t m_entryPoint = 0;
    std::string m_loadedGame;

    // Callbacks
    SystemCallbacks m_callbacks;
    WeaR_Specs m_specs;
};

} // namespace WeaR
