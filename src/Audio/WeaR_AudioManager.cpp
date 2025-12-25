#include "WeaR_AudioManager.h"

#include <QMediaDevices>
#include <QThread>

#include <iostream>
#include <format>
#include <chrono>
#include <thread>

namespace WeaR {

// =============================================================================
// SINGLETON INSTANCE
// =============================================================================

WeaR_AudioManager& WeaR_AudioManager::get() {
    static WeaR_AudioManager instance;
    return instance;
}

// =============================================================================
// CONSTRUCTOR / DESTRUCTOR
// =============================================================================

WeaR_AudioManager::WeaR_AudioManager() : QObject(nullptr) {
    // Setup format
    m_format.setSampleRate(AudioConstants::SAMPLE_RATE);
    m_format.setChannelCount(AudioConstants::CHANNELS);
    m_format.setSampleFormat(QAudioFormat::Int16);
}

WeaR_AudioManager::~WeaR_AudioManager() {
    shutdown();
}

// =============================================================================
// INITIALIZATION
// =============================================================================

bool WeaR_AudioManager::init() {
    if (m_initialized) return true;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Get default audio output device
    m_outputDevice = QMediaDevices::defaultAudioOutput();
    
    if (m_outputDevice.isNull()) {
        std::cerr << "[Audio] No audio output device found\n";
        // Continue anyway - we'll just not output audio
    } else {
        std::cout << std::format("[Audio] Using device: {}\n", 
                                  m_outputDevice.description().toStdString());
    }
    
    // Check format support
    if (!m_outputDevice.isNull() && !m_outputDevice.isFormatSupported(m_format)) {
        std::cerr << "[Audio] Default format not supported, trying nearest\n";
        // Qt will use nearest supported format
    }
    
    m_initialized = true;
    std::cout << std::format("[Audio] Initialized ({}Hz, {} channels, 16-bit)\n",
                              AudioConstants::SAMPLE_RATE, AudioConstants::CHANNELS);
    return true;
}

void WeaR_AudioManager::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Close all ports
    for (auto& [handle, port] : m_ports) {
        if (port && port->sink) {
            port->sink->stop();
        }
    }
    m_ports.clear();
    
    m_initialized = false;
}

// =============================================================================
// PORT MANAGEMENT
// =============================================================================

int32_t WeaR_AudioManager::allocateHandle() {
    return m_nextHandle++;
}

AudioPort* WeaR_AudioManager::getPort(int32_t handle) {
    auto it = m_ports.find(handle);
    if (it != m_ports.end()) {
        return it->second.get();
    }
    return nullptr;
}

int32_t WeaR_AudioManager::openPort(
    int32_t type,
    int32_t index,
    int32_t sampleCount,
    int32_t sampleRate,
    uint32_t paramType)
{
    (void)index;
    (void)paramType;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_initialized) {
        init();
    }
    
    auto port = std::make_unique<AudioPort>();
    port->handle = allocateHandle();
    port->type = type;
    port->sampleCount = sampleCount > 0 ? sampleCount : 256;
    port->grain = port->sampleCount;
    port->isOpen = true;
    port->volume = m_masterVolume;
    
    // Create audio sink if we have a valid device
    if (!m_outputDevice.isNull()) {
        QAudioFormat format = m_format;
        if (sampleRate > 0 && sampleRate != AudioConstants::SAMPLE_RATE) {
            format.setSampleRate(sampleRate);
        }
        
        port->sink = std::make_unique<QAudioSink>(m_outputDevice, format);
        port->sink->setBufferSize(port->sampleCount * AudioConstants::FRAME_SIZE * 4);
        port->ioDevice = port->sink->start();
        
        if (!port->ioDevice) {
            std::cerr << std::format("[Audio] Failed to start sink for handle {}\n", port->handle);
        }
    }
    
    int32_t handle = port->handle;
    m_ports[handle] = std::move(port);
    
    std::cout << std::format("[Audio] Opened port: handle={}, type={}, samples={}\n",
                              handle, type, sampleCount);
    
    emit portOpened(handle);
    return handle;
}

int32_t WeaR_AudioManager::closePort(int32_t handle) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_ports.find(handle);
    if (it == m_ports.end()) {
        return -1;  // Invalid handle
    }
    
    if (it->second->sink) {
        it->second->sink->stop();
    }
    
    m_ports.erase(it);
    emit portClosed(handle);
    
    return 0;
}

// =============================================================================
// AUDIO OUTPUT
// =============================================================================

int32_t WeaR_AudioManager::output(int32_t handle, const void* pcmData, size_t dataSize) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    AudioPort* port = getPort(handle);
    if (!port || !port->isOpen) {
        return -1;
    }
    
    // Write audio data
    if (port->ioDevice && pcmData && dataSize > 0 && !m_masterMuted && !port->isMuted) {
        // Apply volume if needed (for simplicity, we output raw PCM)
        qint64 written = port->ioDevice->write(
            static_cast<const char*>(pcmData), 
            static_cast<qint64>(dataSize)
        );
        
        if (written > 0) {
            uint64_t frames = written / AudioConstants::FRAME_SIZE;
            port->framesOutput += frames;
            m_totalFramesOutput += frames;
        }
    }
    
    // Simulate blocking behavior - sleep for audio duration
    // This prevents the game from running audio too fast
    if (port->sampleCount > 0) {
        // Calculate duration in microseconds
        double durationMs = (static_cast<double>(port->sampleCount) / AudioConstants::SAMPLE_RATE) * 1000.0;
        // Sleep for ~80% of the duration to account for processing time
        std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int>(durationMs * 800)));
    }
    
    return 0;
}

int32_t WeaR_AudioManager::setVolume(int32_t handle, float volume) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    AudioPort* port = getPort(handle);
    if (!port) {
        return -1;
    }
    
    port->volume = std::clamp(volume, 0.0f, 1.0f);
    
    // Apply to sink if available
    if (port->sink) {
        port->sink->setVolume(port->volume * m_masterVolume);
    }
    
    return 0;
}

int32_t WeaR_AudioManager::getPortParam(int32_t handle, int32_t* outSampleCount, int32_t* outGrain) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    AudioPort* port = getPort(handle);
    if (!port) {
        return -1;
    }
    
    if (outSampleCount) *outSampleCount = port->sampleCount;
    if (outGrain) *outGrain = port->grain;
    
    return 0;
}

// =============================================================================
// GLOBAL CONTROLS
// =============================================================================

void WeaR_AudioManager::setMasterMute(bool muted) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_masterMuted = muted;
    
    // Apply to all sinks
    for (auto& [handle, port] : m_ports) {
        if (port->sink) {
            port->sink->setVolume(muted ? 0.0f : port->volume * m_masterVolume);
        }
    }
}

void WeaR_AudioManager::setMasterVolume(float volume) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_masterVolume = std::clamp(volume, 0.0f, 1.0f);
    
    // Apply to all sinks
    for (auto& [handle, port] : m_ports) {
        if (port->sink) {
            port->sink->setVolume(port->volume * m_masterVolume);
        }
    }
}

// =============================================================================
// STATISTICS
// =============================================================================

size_t WeaR_AudioManager::getOpenPortCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_ports.size();
}

} // namespace WeaR
