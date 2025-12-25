#include "WeaR_Logger.h"

#include <QDateTime>
#include <iostream>

namespace WeaR {

// =============================================================================
// GLOBAL LOGGER
// =============================================================================

WeaR_Logger& getLogger() {
    static WeaR_Logger instance;
    return instance;
}

// =============================================================================
// CONSTRUCTOR
// =============================================================================

WeaR_Logger::WeaR_Logger(QObject* parent)
    : QObject(parent)
{
}

// =============================================================================
// LOGGING
// =============================================================================

void WeaR_Logger::log(const std::string& message, LogLevel level) {
    log(QString::fromStdString(message), level);
}

void WeaR_Logger::log(const QString& message, LogLevel level) {
    QString formatted = formatMessage(message, level);
    
    {
        QMutexLocker locker(&m_mutex);
        m_pendingMessages.enqueue(formatted);
        m_messageCount++;
    }

    // Emit signal (will be queued if cross-thread)
    emit logAdded(formatted, static_cast<int>(level));

    // Also output to console
    std::cout << formatted.toStdString() << "\n";
}

QString WeaR_Logger::formatMessage(const QString& message, LogLevel level) const {
    QString prefix;
    switch (level) {
        case LogLevel::Debug:   prefix = "[DBG]"; break;
        case LogLevel::Info:    prefix = "[INF]"; break;
        case LogLevel::Warning: prefix = "[WRN]"; break;
        case LogLevel::Error:   prefix = "[ERR]"; break;
        case LogLevel::Syscall: prefix = "[SYS]"; break;
    }

    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    return QString("%1 %2 %3").arg(timestamp, prefix, message);
}

void WeaR_Logger::clear() {
    QMutexLocker locker(&m_mutex);
    m_pendingMessages.clear();
}

QStringList WeaR_Logger::flushMessages() {
    QMutexLocker locker(&m_mutex);
    QStringList messages;
    while (!m_pendingMessages.isEmpty()) {
        messages.append(m_pendingMessages.dequeue());
    }
    return messages;
}

bool WeaR_Logger::hasPending() const {
    QMutexLocker locker(&m_mutex);
    return !m_pendingMessages.isEmpty();
}

} // namespace WeaR
