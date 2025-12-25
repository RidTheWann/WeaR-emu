#pragma once

/**
 * @file WeaR_Logger.h
 * @brief Thread-safe logging system for kernel output
 * 
 * Allows CPU thread to safely push log messages to GUI.
 * Uses Qt signals/slots for thread-safe updates.
 */

#include <QObject>
#include <QString>
#include <QMutex>
#include <QQueue>

namespace WeaR {

/**
 * @brief Log message severity
 */
enum class LogLevel : uint8_t {
    Debug,
    Info,
    Warning,
    Error,
    Syscall
};

/**
 * @brief Thread-safe kernel logger
 */
class WeaR_Logger : public QObject {
    Q_OBJECT

public:
    explicit WeaR_Logger(QObject* parent = nullptr);
    ~WeaR_Logger() override = default;

    /**
     * @brief Log a message (thread-safe)
     * @param message Message to log
     * @param level Severity level
     */
    void log(const std::string& message, LogLevel level = LogLevel::Info);

    /**
     * @brief Log with QString (thread-safe)
     */
    void log(const QString& message, LogLevel level = LogLevel::Info);

    /**
     * @brief Clear all logs
     */
    void clear();

    /**
     * @brief Get pending messages (call from GUI thread)
     */
    QStringList flushMessages();

    /**
     * @brief Check if there are pending messages
     */
    [[nodiscard]] bool hasPending() const;

    /**
     * @brief Get total message count
     */
    [[nodiscard]] uint64_t getMessageCount() const { return m_messageCount; }

signals:
    /**
     * @brief Emitted when new log is available (use queued connection)
     */
    void logAdded(const QString& message, int level);

private:
    QString formatMessage(const QString& message, LogLevel level) const;

    mutable QMutex m_mutex;
    QQueue<QString> m_pendingMessages;
    uint64_t m_messageCount = 0;
};

/**
 * @brief Get global logger instance
 */
WeaR_Logger& getLogger();

} // namespace WeaR
