#include "WeaR_System.h"
#include "Graphics/WeaR_RenderEngine.h"
#include "HLE/WeaR_Syscalls.h"

#include <iostream>
#include <format>

namespace WeaR {

// =============================================================================
// CONSTRUCTOR / DESTRUCTOR
// =============================================================================

WeaR_System::WeaR_System() = default;

WeaR_System::~WeaR_System() {
    shutdown();
}

// =============================================================================
// INITIALIZATION
// =============================================================================

bool WeaR_System::initialize(const WeaR_Specs& specs) {
    if (m_state.load() != SystemState::Uninitialized) {
        std::cout << "[System] Already initialized\n";
        return false;
    }

    m_specs = specs;

    try {
        std::cout << "[System] Initializing subsystems...\n";

        // Initialize memory
        m_memory = std::make_unique<WeaR_Memory>();
        if (!m_memory->isInitialized()) {
            std::cerr << "[System] Failed to initialize memory\n";
            setState(SystemState::Error);
            return false;
        }
        std::cout << "[System] Memory initialized\n";

        // Initialize CPU
        m_cpu = std::make_unique<WeaR_Cpu>(*m_memory);
        
        // Set syscall handler
        m_cpu->setSyscallHandler([this](WeaR_Context& ctx) {
            handleSyscall(ctx);
        });
        std::cout << "[System] CPU initialized\n";

        // Initialize ELF loader
        m_elfLoader = std::make_unique<WeaR_ElfLoader>();
        std::cout << "[System] ELF loader initialized\n";

        setState(SystemState::Ready);
        std::cout << "[System] All subsystems ready\n";
        return true;

    } catch (const std::exception& e) {
        std::cerr << std::format("[System] Init failed: {}\n", e.what());
        setState(SystemState::Error);
        return false;
    }
}

// =============================================================================
// GAME LOADING
// =============================================================================

uint64_t WeaR_System::loadGame(const std::string& filepath) {
    if (!m_memory || !m_elfLoader) {
        std::cerr << "[System] Not initialized\n";
        return 0;
    }

    std::cout << std::format("[System] Loading game: {}\n", filepath);

    auto result = m_elfLoader->loadElf(filepath, *m_memory);
    if (!result) {
        std::cerr << std::format("[System] Load failed: {}\n", result.error());
        return 0;
    }

    m_entryPoint = result->entryPoint;
    m_loadedGame = filepath;

    std::cout << std::format("[System] Game loaded. Entry: 0x{:016X}\n", m_entryPoint);

    // Set up initial CPU state
    m_cpu->getContext().RIP = m_entryPoint;
    m_cpu->getContext().RSP = PS4Memory::Region::STACK_TOP - 8;  // Start near top of stack

    setState(SystemState::Ready);
    return m_entryPoint;
}

// =============================================================================
// EXECUTION CONTROL
// =============================================================================

void WeaR_System::boot() {
    if (m_state.load() != SystemState::Ready) {
        std::cerr << "[System] Not ready to boot\n";
        return;
    }

    if (m_entryPoint == 0) {
        std::cerr << "[System] No game loaded\n";
        return;
    }

    std::cout << std::format("[System] Booting at RIP=0x{:016X}\n", m_entryPoint);

    // Stop any existing thread
    if (m_cpuThread && m_cpuThread->joinable()) {
        m_cpu->stop();
        m_cpuThread->join();
    }

    setState(SystemState::Running);

    // Start CPU thread
    m_cpuThread = std::make_unique<std::thread>(&WeaR_System::cpuThreadFunc, this);
}

void WeaR_System::stop() {
    if (m_cpu) {
        m_cpu->stop();
    }

    if (m_cpuThread && m_cpuThread->joinable()) {
        m_cpuThread->join();
    }

    setState(SystemState::Stopped);
}

void WeaR_System::pause() {
    if (m_cpu && m_state.load() == SystemState::Running) {
        m_cpu->pause();
        setState(SystemState::Paused);
    }
}

void WeaR_System::resume() {
    if (m_cpu && m_state.load() == SystemState::Paused) {
        m_cpu->resume();
        setState(SystemState::Running);
    }
}

void WeaR_System::reset() {
    stop();
    
    if (m_cpu) {
        m_cpu->reset();
    }

    m_entryPoint = 0;
    m_loadedGame.clear();

    setState(SystemState::Ready);
}

void WeaR_System::shutdown() {
    stop();

    m_cpu.reset();
    m_memory.reset();
    m_elfLoader.reset();
    m_renderer = nullptr;

    setState(SystemState::Uninitialized);
}

// =============================================================================
// CPU THREAD
// =============================================================================

void WeaR_System::cpuThreadFunc() {
    std::cout << "[System] CPU thread started\n";

    try {
        m_cpu->runLoop();
    } catch (const std::exception& e) {
        std::cerr << std::format("[System] CPU thread exception: {}\n", e.what());
        setState(SystemState::Error);
    }

    // Update state based on CPU state
    CpuState cpuState = m_cpu->getState();
    switch (cpuState) {
        case CpuState::Halted:
            setState(SystemState::Stopped);
            break;
        case CpuState::Faulted:
            setState(SystemState::Error);
            break;
        default:
            if (m_state.load() != SystemState::Error) {
                setState(SystemState::Stopped);
            }
    }

    std::cout << "[System] CPU thread ended\n";
}

// =============================================================================
// STATE ACCESS
// =============================================================================

WeaR_Context WeaR_System::getCpuSnapshot() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_cpuMutex));
    if (m_cpu) {
        return m_cpu->getContext();
    }
    return {};
}

uint64_t WeaR_System::getInstructionCount() const {
    if (m_cpu) {
        return m_cpu->getInstructionCount();
    }
    return 0;
}

void WeaR_System::setState(SystemState state) {
    SystemState oldState = m_state.exchange(state);
    if (oldState != state && m_callbacks.onStateChanged) {
        m_callbacks.onStateChanged(state);
    }
}

// =============================================================================
// SYSCALL HANDLER
// =============================================================================

void WeaR_System::handleSyscall(WeaR_Context& ctx) {
    // Route to global HLE syscall dispatcher
    getSyscallDispatcher().dispatch(ctx, *m_memory);
    
    // Check if sys_exit was called
    if (ctx.RAX == 0 && getSyscallDispatcher().getTotalCalls() > 0) {
        // Could check for exit syscall here
    }
}

} // namespace WeaR
