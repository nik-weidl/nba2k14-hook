#include "logger.h"
#include <cstdarg>
#include <cstdio>
#include <ctime>

namespace Core {
    std::ofstream Logger::s_LogFile;
    std::mutex Logger::s_Mutex;
    bool Logger::s_Initialized = false;

    void Logger::Init() {
        std::lock_guard<std::mutex> lock(s_Mutex);
        if (s_Initialized) return;

        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        std::string exePath(path);
        size_t lastSlash = exePath.find_last_of("\\/");
        std::string logPath = (lastSlash != std::string::npos ? exePath.substr(0, lastSlash + 1) : "") + "nba2k14_hook.log";

        s_LogFile.open(logPath, std::ios::out | std::ios::trunc);
        s_Initialized = true;

        if (s_LogFile.is_open()) {
            s_LogFile << "=== NBA 2K14 Native Hook Log Started ===" << std::endl;
        }
    }

    void Logger::Shutdown() {
        std::lock_guard<std::mutex> lock(s_Mutex);
        if (s_LogFile.is_open()) {
            s_LogFile << "=== NBA 2K14 Native Hook Log Closed ===" << std::endl;
            s_LogFile.close();
        }
        s_Initialized = false;
    }

    void Logger::Log(const char* format, ...) {
        std::lock_guard<std::mutex> lock(s_Mutex);
        if (!s_Initialized || !s_LogFile.is_open()) return;

        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        // Get timestamp
        time_t now = time(nullptr);
        tm localTime;
        localtime_s(&localTime, &now);
        char timeBuf[32];
        strftime(timeBuf, sizeof(timeBuf), "[%H:%M:%S] ", &localTime);

        s_LogFile << timeBuf << buffer << std::endl;
        s_LogFile.flush();

        OutputDebugStringA(buffer);
        OutputDebugStringA("\n");
    }

    void Logger::LogError(const char* format, ...) {
        std::lock_guard<std::mutex> lock(s_Mutex);
        if (!s_Initialized || !s_LogFile.is_open()) return;

        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        time_t now = time(nullptr);
        tm localTime;
        localtime_s(&localTime, &now);
        char timeBuf[32];
        strftime(timeBuf, sizeof(timeBuf), "[%H:%M:%S] ", &localTime);

        s_LogFile << timeBuf << buffer << std::endl;
        s_LogFile.flush();

        OutputDebugStringA(buffer);
        OutputDebugStringA("\n");
    }
}
