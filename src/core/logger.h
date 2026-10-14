#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <windows.h>

namespace Core {
    class Logger {
    public:
        static void Init();
        static void Shutdown();
        static void Log(const char* format, ...);
        static void LogError(const char* format, ...);

    private:
        static std::ofstream s_LogFile;
        static std::mutex s_Mutex;
        static bool s_Initialized;
    };
}

#define LOG_INFO(fmt, ...) Core::Logger::Log("[INFO] " fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) Core::Logger::Log("[WARN] " fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) Core::Logger::LogError("[ERROR] " fmt, ##__VA_ARGS__)
