//----------------------------------------------------------------------------------
// File:        Logger.h
// Project:     RTX DLSS-5 Video Studio
// Purpose:     Thread-safe, high-performance asynchronous logging
//----------------------------------------------------------------------------------

#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <sstream>
#include <iostream>
#include <chrono>
#include <iomanip>

enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Fatal = 4
};

class Logger {
public:
    static Logger& Instance() {
        static Logger instance;
        return instance;
    }

    void Initialize(const std::wstring& logFilePath, bool enableConsole = true, LogLevel minLevel = LogLevel::Info);
    void Shutdown();

    void Log(LogLevel level, const std::string& message, const char* file = nullptr, int line = 0);
    void Log(LogLevel level, const std::wstring& message, const char* file = nullptr, int line = 0);

    // Audit trail logging specifically for compliance
    void LogAudit(const std::string& action, const std::string& user, const std::string& details);

private:
    Logger() = default;
    ~Logger() { Shutdown(); }

    std::string GetTimestamp();
    const char* LevelToString(LogLevel level);

    std::mutex m_mutex;
    std::ofstream m_fileStream;
    bool m_consoleEnabled = true;
    LogLevel m_minLevel = LogLevel::Info;
    bool m_initialized = false;
};

#define LOG_DEBUG(msg) Logger::Instance().Log(LogLevel::Debug, msg, __FILE__, __LINE__)
#define LOG_INFO(msg)  Logger::Instance().Log(LogLevel::Info,  msg, __FILE__, __LINE__)
#define LOG_WARN(msg)  Logger::Instance().Log(LogLevel::Warning, msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::Instance().Log(LogLevel::Error, msg, __FILE__, __LINE__)
#define LOG_FATAL(msg) Logger::Instance().Log(LogLevel::Fatal, msg, __FILE__, __LINE__)
