//----------------------------------------------------------------------------------
// File:        Logger.cpp
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Logger implementation with ISO8601 timestamps and audit formatting
//----------------------------------------------------------------------------------

#include "Logger.h"
#if defined(_WIN32)
#include <windows.h>
#endif

namespace {
    std::string WideToUtf8(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();
#if defined(_WIN32)
        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        std::string str(sizeNeeded, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), &str[0], sizeNeeded, nullptr, nullptr);
        return str;
#else
        return std::string(wstr.begin(), wstr.end());
#endif
    }
}

void Logger::Initialize(const std::wstring& logFilePath, bool enableConsole, LogLevel minLevel) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_consoleEnabled = enableConsole;
    m_minLevel = minLevel;

    if (m_fileStream.is_open()) {
        m_fileStream.flush();
        m_fileStream.close();
    }
    m_fileStream.clear();

    if (!logFilePath.empty()) {
        std::string narrowPath = WideToUtf8(logFilePath);
        m_fileStream.open(narrowPath, std::ios::out | std::ios::app);
    }
    m_initialized = true;
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_fileStream.is_open()) {
        m_fileStream.flush();
        m_fileStream.close();
    }
    m_initialized = false;
}

std::string Logger::GetTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm timeInfo;
#if defined(_WIN32)
    localtime_s(&timeInfo, &in_time_t);
#else
    localtime_r(&in_time_t, &timeInfo);
#endif

    std::ostringstream ss;
    ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S") << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

const char* Logger::LevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:                return "UNKN ";
    }
}

void Logger::Log(LogLevel level, const std::string& message, const char* file, int line) {
    if (level < m_minLevel) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    std::string timestamp = GetTimestamp();

    std::ostringstream ss;
    ss << "[" << timestamp << "] [" << LevelToString(level) << "] " << message;
    if (file && level >= LogLevel::Error) {
        ss << " (" << file << ":" << line << ")";
    }
    ss << "\n";

    std::string formatted = ss.str();

    if (m_consoleEnabled) {
        if (level >= LogLevel::Error) {
            std::cerr << formatted;
        } else {
            std::cout << formatted;
        }
    }

    if (m_fileStream.is_open()) {
        m_fileStream << formatted;
        m_fileStream.flush();
    }
}

void Logger::Log(LogLevel level, const std::wstring& message, const char* file, int line) {
    Log(level, WideToUtf8(message), file, line);
}

void Logger::LogAudit(const std::string& action, const std::string& user, const std::string& details) {
    std::ostringstream ss;
    ss << "[AUDIT] Action: " << action << " | Operator: " << user << " | Context: " << details;
    Log(LogLevel::Info, ss.str());
}
