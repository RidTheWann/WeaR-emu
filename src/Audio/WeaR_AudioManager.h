#pragma once

/**
 * @file WeaR_AudioManager.h
 * @brief PS4 Audio Output Emulation via Qt6 Multimedia
 * 
 * Handles sceAudioOut syscalls and provides PCM audio output.
 */

#include <QObject>
#include <QAudioSink>
#include <QAudioFormat>
#include <QAudioDevice>
#include <QIODevice>

#include <map>
#include <mutex>
#include <memory>
#include <cstdint>
#include <atomic>

namespace WeaR {

// =============================================================================
// PS4 AUDIO CONSTANTS
// =============================================================================

namespace AudioConstants {
    constexpr int SAMPLE_RATE = 48000;      // PS4 standard
    constexpr int CHANNELS = 2;              // Stereo
    constexpr int BITS_PER_SAMPLE = 16;      // 16-bit signed integer
    constexpr int BYTES_PER_SAMPLE = 2;
    constexpr int FRAME_SIZE = CHANNELS * BYTES_PER_SAMPLE;  // 4 bytes per frame
    
    // Audio port types
    constexpr int32_t AUDIOOUT_PORT_TYPE_MAIN = 0;
    constexpr int32_t AUDIOOUT_PORT_TYPE_BGM = 1;
    constexpr int32_t AUDIOOUT_PORT_TYPE_VOICE = 2;
    constexpr int32_t AUDIOOUT_PORT_TYPE_PERSONAL = 3;
    constexpr int32_t AUDIOOUT_PORT_TYPE_PADSPK = 4;
}

// =============================================================================
// AUDIO PORT STRUCTURE
// =============================================================================

struct AudioPort {
    int32_t handle = -1;
    int32_t type = 0;
    int32_t sampleCount = 0;    // Samples per buffer
    int32_t grain = 256;        // Typical PS4 grain size
    
    std::unique_ptr<QAudioSink> sink;
    QIODevice* ioDevice = nullptr;
    
    bool isOpen = false;
    bool isMuted = false;
    float volume = 1.0f;
    
    uint64_t framesOutput = 0;
};

// =============================================================================
// AUDIO MANAGER CLASS
// =============================================================================

/**
 * @brief Audio Manager singleton (QObject for Qt integration)
 */
class WeaR_AudioManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Get singleton instance
     */
    static WeaR_AudioManager& get();

    /**
     * @brief Initialize audio subsystem
     */
    bool init();

    /**
     * @brief Shutdown audio subsystem
     */
    void shutdown();

    // =========================================================================
    // SCE AUDIO OUT API
    // =========================================================================

    /**
     * @brief Open an audio port
     * @param type Port type (MAIN, BGM, VOICE, etc.)
     * @param index Port index
     * @param sampleCount Samples per output call
     * @param sampleRate Sample rate (usually 48000)
     * @param paramType Additional parameters
     * @return Handle (positive) or error code (negative)
     */
    [[nodiscard]] int32_t openPort(
        int32_t type,
        int32_t index,
        int32_t sampleCount,
        int32_t sampleRate,
        uint32_t paramType
    );

    /**
     * @brief Close an audio port
     */
    int32_t closePort(int32_t handle);

    /**
     * @brief Output audio data
     * @param handle Port handle
     * @param pcmData Pointer to PCM data (16-bit stereo)
     * @param dataSize Size in bytes
     * @return 0 on success, negative error code on failure
     */
    int32_t output(int32_t handle, const void* pcmData, size_t dataSize);

    /**
     * @brief Set port volume
     * @param handle Port handle
     * @param volume Volume (0.0 - 1.0)
     */
    int32_t setVolume(int32_t handle, float volume);

    /**
     * @brief Get port parameters
     */
    int32_t getPortParam(int32_t handle, int32_t* outSampleCount, int32_t* outGrain);

    // =========================================================================
    // GLOBAL CONTROLS
    // =========================================================================

    /**
     * @brief Set master mute
     */
    void setMasterMute(bool muted);
    [[nodiscard]] bool isMasterMuted() const { return m_masterMuted; }

    /**
     * @brief Set master volume (0.0 - 1.0)
     */
    void setMasterVolume(float volume);
    [[nodiscard]] float getMasterVolume() const { return m_masterVolume; }

    // =========================================================================
    // STATISTICS
    // =========================================================================

    [[nodiscard]] size_t getOpenPortCount() const;
    [[nodiscard]] uint64_t getTotalFramesOutput() const { return m_totalFramesOutput; }
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

signals:
    void audioError(const QString& message);
    void portOpened(int32_t handle);
    void portClosed(int32_t handle);

private:
    WeaR_AudioManager();
    ~WeaR_AudioManager();

    [[nodiscard]] int32_t allocateHandle();
    [[nodiscard]] AudioPort* getPort(int32_t handle);

    std::map<int32_t, std::unique_ptr<AudioPort>> m_ports;
    mutable std::mutex m_mutex;
    
    QAudioFormat m_format;
    QAudioDevice m_outputDevice;
    
    bool m_initialized = false;
    bool m_masterMuted = false;
    float m_masterVolume = 1.0f;
    
    int32_t m_nextHandle = 1;
    std::atomic<uint64_t> m_totalFramesOutput{0};
};

} // namespace WeaR
