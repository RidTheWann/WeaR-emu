#include "WeaR_EmulatorCore.h"
#include "WeaR_Memory.h"
#include "WeaR_Cpu.h"
#include "WeaR_InternalBios.h"
#include "Loader/WeaR_ElfLoader.h"
#include "HLE/WeaR_Syscalls.h"
#include "HLE/FileSystem/WeaR_VFS.h"
#include "Audio/WeaR_AudioManager.h"
#include "Input/WeaR_Input.h"
#include "Graphics/WeaR_RenderEngine.h"

#include <iostream>
#include <format>
#include <filesystem>

namespace WeaR {

// =============================================================================
// STATE NAME HELPER
// =============================================================================

const char* getEmuStateName(EmuState state) {
    switch (state) {
        case EmuState::Idle:     return "IDLE";
        case EmuState::Booting:  return "BOOTING";
        case EmuState::Running:  return "RUNNING";
        case EmuState::Paused:   return "PAUSED";
        case EmuState::Stopping: return "STOPPING";
        case EmuState::Error:    return "ERROR";
        default:                 return "UNKNOWN";
    }
}

// =============================================================================
// GLOBAL INSTANCE
// =============================================================================

WeaR_EmulatorCore& getEmulatorCore() {
    static WeaR_EmulatorCore instance;
    return instance;
}

// =============================================================================
// CONSTRUCTOR / DESTRUCTOR
// =============================================================================

WeaR_EmulatorCore::WeaR_EmulatorCore() {
    std::cout << "[Core] EmulatorCore created\n";
}

WeaR_EmulatorCore::~WeaR_EmulatorCore() {
    shutdown();
}

// =============================================================================
// INITIALIZATION
// =============================================================================

bool WeaR_EmulatorCore::initialize(const WeaR_Specs& specs) {
    if (m_initialized) return true;
    
    log("Initializing EmulatorCore...");
    
    setState(EmuState::Booting);
    
    // 1. Initialize Memory (constructor allocates)
    m_memory = std::make_unique<WeaR_Memory>();
    if (!m_memory->isInitialized()) {
        log("Failed to initialize memory");
        setState(EmuState::Error);
        return false;
    }
    
    // 2. Initialize CPU (requires memory reference)
    m_cpu = std::make_unique<WeaR_Cpu>(*m_memory);
    
    // 3. Setup syscall handler
    m_cpu->setSyscallHandler([this](WeaR_Context& ctx) {
        getSyscallDispatcher().dispatch(ctx, *m_memory);
    });
    
    // 4. Initialize HLE modules
    initializeHLE();
    
    // 5. Initialize Audio
    WeaR_AudioManager::get().init();
    
    // 6. Reset Input
    WeaR_InputManager::get().reset();
    
    (void)specs;  // Specs used by renderer
    
    m_initialized = true;
    setState(EmuState::Idle);
    log("EmulatorCore initialized successfully");
    
    return true;
}

void WeaR_EmulatorCore::shutdown() {
    if (!m_initialized) return;
    
    log("Shutting down EmulatorCore...");
    
    // Stop CPU first
    stop();
    
    // Shutdown Audio
    WeaR_AudioManager::get().shutdown();
    
    // Clear VFS
    WeaR_VFS::get().clearMounts();
    
    // Clear CPU and Memory
    m_cpu.reset();
    m_memory.reset();
    
    m_initialized = false;
    m_gameLoaded = false;
    m_entryPoint = 0;
    m_gamePath.clear();
    
    setState(EmuState::Idle);
    log("EmulatorCore shutdown complete");
}

void WeaR_EmulatorCore::initializeHLE() {
    // HLE modules register their handlers when included
    log("HLE modules loaded");
}

// =============================================================================
// GAME LOADING
// =============================================================================

uint64_t WeaR_EmulatorCore::loadGame(const std::string& path) {
    if (!m_initialized) {
        log("Cannot load game: not initialized");
        return 0;
    }
    
    if (m_state != EmuState::Idle) {
        log("Cannot load game: not idle");
        return 0;
    }
    
    setState(EmuState::Booting);
    log(std::format("Loading game: {}", path));
    
    // Mount /app0 to game directory
    std::filesystem::path gamePath = path;
    std::string gameDir = gamePath.parent_path().string();
    WeaR_VFS::get().mount("/app0", gameDir);
    WeaR_VFS::get().mount("/hostapp", gameDir);
    
    // Load ELF using loadElf() 
    WeaR_ElfLoader loader;
    auto result = loader.loadElf(gamePath, *m_memory);
    
    if (!result) {
        log(std::format("Failed to load ELF: {}", result.error()));
        setState(EmuState::Idle);
        return 0;
    }
    
    m_entryPoint = result->entryPoint;
    m_gamePath = path;
    m_gameLoaded = true;
    
    // Set CPU entry point
    auto& ctx = m_cpu->getContext();
    ctx.RIP = m_entryPoint;
    ctx.RSP = PS4Memory::Region::STACK_TOP - 0x1000;
    ctx.RBP = ctx.RSP;
    
    log(std::format("Game loaded. Entry: 0x{:X}", m_entryPoint));
    setState(EmuState::Idle);
    
    return m_entryPoint;
}

uint64_t WeaR_EmulatorCore::loadInternalBios() {
    if (!m_initialized) {
        log("Cannot load BIOS: not initialized");
        return 0;
    }
    
    log("Loading Internal BIOS...");
    setState(EmuState::Booting);
    
    m_entryPoint = InternalBios::load(*m_memory, m_cpu->getContext());
    m_gamePath = "[Internal BIOS]";
    m_gameLoaded = true;
    
    log(std::format("Internal BIOS loaded. Entry: 0x{:X}", m_entryPoint));
    setState(EmuState::Idle);
    
    return m_entryPoint;
}

// =============================================================================
// STATE CONTROL
// =============================================================================

bool WeaR_EmulatorCore::run() {
    EmuState current = m_state.load();
    
    if (current != EmuState::Idle && current != EmuState::Paused) {
        return false;
    }
    
    if (!m_gameLoaded) {
        log("Cannot run: no game loaded");
        return false;
    }
    
    log("Starting emulation...");
    setState(EmuState::Running);
    
    // Start CPU thread if not running
    if (!m_cpuRunning.load()) {
        m_cpuRunning = true;
        
        if (m_cpuThread.joinable()) {
            m_cpuThread.join();
        }
        
        // Start CPU thread - runLoop() takes no arguments
        m_cpuThread = std::thread([this]() {
            cpuThreadMain();
        });
    } else {
        // Resume paused CPU
        m_cpu->resume();
    }
    
    return true;
}

bool WeaR_EmulatorCore::pause() {
    if (m_state != EmuState::Running) {
        return false;
    }
    
    log("Pausing emulation...");
    m_cpu->pause();
    setState(EmuState::Paused);
    
    return true;
}

bool WeaR_EmulatorCore::stop() {
    EmuState current = m_state.load();
    
    if (current == EmuState::Idle || current == EmuState::Stopping) {
        return false;
    }
    
    log("Stopping emulation...");
    setState(EmuState::Stopping);
    
    // Stop CPU
    m_cpuRunning = false;
    m_cpu->stop();
    
    // Wait for CPU thread
    if (m_cpuThread.joinable()) {
        m_cpuThread.join();
    }
    
    // Reset state
    m_cpu->reset();
    WeaR_InputManager::get().reset();
    
    m_gameLoaded = false;
    m_entryPoint = 0;
    m_gamePath.clear();
    
    setState(EmuState::Idle);
    log("Emulation stopped");
    
    return true;
}

bool WeaR_EmulatorCore::togglePause() {
    if (m_state == EmuState::Running) {
        return pause();
    } else if (m_state == EmuState::Paused) {
        return run();
    }
    return false;
}

// =============================================================================
// CPU THREAD
// =============================================================================

void WeaR_EmulatorCore::cpuThreadMain() {
    log("CPU thread started");
    
    // runLoop() runs until stop() is called
    while (m_cpuRunning.load() && m_state != EmuState::Stopping) {
        if (m_cpu->getState() == CpuState::Paused) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        m_cpu->step();
    }
    
    log("CPU thread exiting");
}

// =============================================================================
// CALLBACKS
// =============================================================================

void WeaR_EmulatorCore::setStateCallback(StateCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_stateCallback = std::move(callback);
}

void WeaR_EmulatorCore::setLogCallback(LogCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_logCallback = std::move(callback);
}

void WeaR_EmulatorCore::setState(EmuState newState) {
    m_state = newState;
    
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    if (m_stateCallback) {
        m_stateCallback(newState);
    }
}

void WeaR_EmulatorCore::log(const std::string& message) {
    std::cout << std::format("[Core] {}\n", message);
    
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    if (m_logCallback) {
        m_logCallback(message);
    }
}

// =============================================================================
// SUBSYSTEM ACCESS
// =============================================================================

WeaR_Context WeaR_EmulatorCore::getCpuSnapshot() const {
    if (m_cpu) {
        return m_cpu->getContext();
    }
    return {};
}

uint64_t WeaR_EmulatorCore::getInstructionCount() const {
    if (m_cpu) {
        return m_cpu->getInstructionCount();
    }
    return 0;
}

void WeaR_EmulatorCore::setRenderer(WeaR_RenderEngine* renderer) {
    m_renderer = renderer;
}

} // namespace WeaR
