#include "WeaR_EmulatorCore.h"
#include "WeaR_Memory.h"
#include "WeaR_Cpu.h"
#include "WeaR_InternalBios.h"
#include "Loader/WeaR_ElfLoader.h"
#include "Loader/WeaR_PkgLoader.h"
#include "HLE/WeaR_Syscalls.h"
#include "HLE/FileSystem/WeaR_VFS.h"
#include "Audio/WeaR_AudioManager.h"
#include "Input/WeaR_Input.h"
#include "Graphics/WeaR_RenderEngine.h"

#include <iostream>
#include <format>
#include <filesystem>
#include <fstream>

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
    
    // UNIVERSAL CRASH GUARD
    try {
    
    // Mount /app0 to game directory
    std::filesystem::path gamePath = path;
    std::string gameDir = gamePath.parent_path().string();
    WeaR_VFS::get().mount("/app0", gameDir);
    WeaR_VFS::get().mount("/hostapp", gameDir);
    
    // Detect file type by magic header
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        log("Failed to open file");
        setState(EmuState::Idle);
        return 0;
    }
    
    uint32_t magic = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.close();
    
    // PKG Magic (Big Endian): 0x7F 0x43 0x4E 0x54 -> reads as 0x544E437F on LE
    constexpr uint32_t PKG_MAGIC_LE = 0x544E437F;  // "\x7FCNT" as little endian
    constexpr uint32_t ELF_MAGIC = 0x464C457F;     // "\x7FELF"
    
    bool isPkg = (magic == PKG_MAGIC_LE);
    bool isElf = (magic == ELF_MAGIC);
    
    if (isPkg) {
        // Load PKG file
        log("Detected PKG format - extracting eboot.bin...");
        WeaR_PkgLoader pkgLoader;
        auto pkgResult = pkgLoader.loadPackage(gamePath);
        if (!pkgResult) {
            log(std::format("Failed to load PKG: {}", pkgResult.error()));
            setState(EmuState::Idle);
            return 0;
        }
        
        auto ebootData = pkgLoader.extractEboot();
        if (!ebootData) {
            log(std::format("Failed to extract eboot: {}", ebootData.error()));
            setState(EmuState::Idle);
            return 0;
        }
        
        log(std::format("Extracted eboot.bin ({} bytes / {} MB)", 
                       ebootData->size(), ebootData->size() / 1024 / 1024));
        
        // Detect extracted file format by magic bytes
        if (ebootData->size() >= 4) {
            uint32_t magic = *reinterpret_cast<const uint32_t*>(ebootData->data());
            log(std::format("Game binary magic: 0x{:08X}", magic));
            
            // ELF magic: 0x464C457F (little endian: 7F 45 4C 46)
            if (magic == 0x464C457F) {
                log("Format: ELF executable detected");
            }
            // PKG magic nested: 0x544E437F
            else if (magic == 0x544E437F) {
                log("Format: Nested PKG detected (PS2 Classic wrapper)");
            }
            // ISO magic: "CD001" at offset 32769
            else if (ebootData->size() > 32800) {
                std::string check(ebootData->begin() + 32769, ebootData->begin() + 32774);
                if (check == "CD001") {
                    log("Format: ISO9660 detected (PS2 disc image)");
                }
            }
            else {
                log(std::format("Format: Unknown (magic: 0x{:08X})", magic));
            }
        }
        
        // ================================================================
        // ISOLATION MODE: BYPASS ELF LOADING FOR CRASH DIAGNOSIS
        // ================================================================
        log("[CORE] =========================================");
        log("[CORE] ISOLATION MODE ACTIVE");
        log("[CORE] ELF Loading BYPASSED - Data is in RAM");
        log(std::format("[CORE] Extracted Data: {} bytes ({} MB)", ebootData->size(), ebootData->size() / 1024 / 1024));
        log("[CORE] =========================================");
        
        // Mark as loaded in isolation/legacy mode
        m_gameLoaded = true;
        m_isLegacyMode = true;  // Force safe mode - NO CPU execution
        m_gamePath = path;
        m_entryPoint = 0x0;
        
        setState(EmuState::Idle);
        return 0x1; // Success - isolation mode
        
        /* ORIGINAL ELF LOADING - DISABLED FOR ISOLATION TEST
        log("Attempting to load as PS4 ELF...");
        WeaR_ElfLoader elfLoader;
        auto result = elfLoader.loadElfFromMemory(*ebootData, *m_memory);
        if (!result) {
            log(std::format("Not a valid PS4 ELF: {}", result.error()));
            // PS2 CLASSIC / LEGACY BYPASS
            if (ebootData->size() > 1024 * 1024) {
                log("[CORE] ⚠️  WARNING: ELF Loader rejected binary. Assuming PS2 Classic / Legacy Mode.");
                log("[CORE] Bypassing ELF execution structure.");
                log(std::format("[CORE] Data loaded in RAM: {} MB", ebootData->size() / 1024 / 1024));
                m_gameLoaded = true;
                m_isLegacyMode = true;
                m_entryPoint = 0x0;
                log("[CORE] ✓ Game data loaded successfully (Legacy/PS2 Classic mode)");
                log("[CORE] Note: This game cannot be executed as it requires PS2 emulation");
                setState(EmuState::Idle);
                return 0x1;
            }
            log("This game may be a PS2 Classic or remastered title that requires different handling");
            setState(EmuState::Idle);
            return 0;
        }
        m_entryPoint = result->entryPoint;
        END OF DISABLED CODE */
        
    } else if (isElf) {
        // Load ELF directly
        log("Detected ELF format");
        WeaR_ElfLoader loader;
        auto result = loader.loadElf(gamePath, *m_memory);
        
        if (!result) {
            log(std::format("Failed to load ELF: {}", result.error()));
            setState(EmuState::Idle);
            return 0;
        }
        m_entryPoint = result->entryPoint;
        
    } else {
        log(std::format("Unknown file format (magic: 0x{:08X})", magic));
        setState(EmuState::Idle);
        return 0;
    }
    
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
    
    } catch (const std::bad_alloc& e) {
        log(std::format("[CORE] CRASH (Out of Memory): {}", e.what()));
        setState(EmuState::Error);
        return 0;
    } catch (const std::exception& e) {
        log(std::format("[CORE] CRASH during Load: {}", e.what()));
        setState(EmuState::Error);
        return 0;
    } catch (...) {
        log("[CORE] UNKNOWN FATAL CRASH during Load!");
        setState(EmuState::Error);
        return 0;
    }
}


uint64_t WeaR_EmulatorCore::loadInternalBios() {
    log("[BIOS] === BIOS LOAD START ===");
    
    // Safety check: must be initialized
    if (!m_initialized) {
        log("[BIOS] ERROR: EmulatorCore not initialized!");
        setState(EmuState::Error);
        return 0;
    }
    log("[BIOS] ✓ Core initialized");
    
    // Safety check: memory subsystem
    if (!m_memory) {
        log("[BIOS] ERROR: Memory subsystem is null!");
        setState(EmuState::Error);
        return 0;
    }
    log("[BIOS] ✓ Memory subsystem OK");
    
    // Safety check: CPU subsystem
    if (!m_cpu) {
        log("[BIOS] ERROR: CPU subsystem is null!");
        setState(EmuState::Error);
        return 0;
    }
    log("[BIOS] ✓ CPU subsystem OK");
    
    log("[BIOS] Attempting to load Internal BIOS...");
    setState(EmuState::Booting);
    
    try {
        log("[BIOS] Calling InternalBios::load()...");
        
        // Get CPU context reference
        WeaR_Context& ctx = m_cpu->getContext();
        log("[BIOS] ✓ Got CPU context reference");
        
        // Load BIOS into memory
        m_entryPoint = InternalBios::load(*m_memory, ctx);
        log(std::format("[BIOS] ✓ BIOS code written to memory, entry: 0x{:X}", m_entryPoint));
        
        m_gamePath = "[Internal BIOS]";
        m_gameLoaded = true;
        
        log("[BIOS] ✓ Internal BIOS loaded successfully!");
        setState(EmuState::Idle);
        
        return m_entryPoint;
        
    } catch (const std::bad_alloc& e) {
        log(std::format("[BIOS] EXCEPTION (bad_alloc): {}", e.what()));
        log("[BIOS] Out of memory!");
        setState(EmuState::Error);
        return 0;
        
    } catch (const std::exception& e) {
        log(std::format("[BIOS] EXCEPTION (std::exception): {}", e.what()));
        setState(EmuState::Error);
        return 0;
        
    } catch (...) {
        log("[BIOS] UNKNOWN EXCEPTION during BIOS load!");
        log("[BIOS] This usually means a memory access violation in InternalBios::load()");
        setState(EmuState::Error);
        return 0;
    }
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
    
    // Safety check: CPU must be initialized
    if (!m_cpu) {
        log("[ERROR] Cannot run: CPU subsystem is null!");
        setState(EmuState::Error);
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
    log("[CPU] =========================================");
    log("[CPU] ISOLATION MODE - NO EXECUTION");
    log("[CPU] CPU Thread is SLEEPING ONLY");
    log("[CPU] =========================================");
    
    // ISOLATION MODE: Pure idle loop - NO CPU EXECUTION AT ALL
    while (m_cpuRunning.load() && m_state != EmuState::Stopping) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS idle
    }
    
    log("[CPU] Thread exiting safely (Isolation Mode)");
    
    /* ORIGINAL CPU LOOP - DISABLED FOR ISOLATION TEST
    while (m_cpuRunning.load() && m_state != EmuState::Stopping) {
        try {
            if (m_isLegacyMode) {
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                continue;
            }
            if (m_cpu && m_cpu->getState() == CpuState::Paused) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            if (m_cpu) {
                m_cpu->step();
            }
        } catch (...) {
            log("[CPU] CRASH - Switching to Safe Mode");
            m_isLegacyMode = true;
        }
    }
    END OF DISABLED CODE */
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
