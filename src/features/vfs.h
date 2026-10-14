#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>

namespace Features {
    struct ModFileInfo {
        std::string filename;      // e.g. "face1001.iff"
        std::string relativePath;  // e.g. "jerseys/face1001.iff"
        std::string fullPath;      // e.g. "S:\SteamLibrary\...\mods\jerseys\face1001.iff"
        uint64_t fileSize;         // in bytes
    };

    class VFS {
    public:
        static VFS& Get();

        void Init();
        void Rescan();
        std::string GetRedirectPath(const char* originalPath);
        void OpenFolderInExplorer();

        bool IsEnabled() const { return m_Enabled; }
        void SetEnabled(bool enabled) { m_Enabled = enabled; }

        const std::vector<ModFileInfo>& GetLoadedMods() const { return m_ModList; }
        const std::string& GetModsDirectory() const { return m_ModsDirectory; }
        size_t GetModCount() const { return m_ModList.size(); }
        size_t GetTotalRedirections() const { return m_TotalRedirections; }

    private:
        VFS() = default;
        void ScanDirectoryRecursive(const std::string& directory, const std::string& subDir);

        bool m_Enabled = true;
        std::string m_ModsDirectory;
        std::unordered_map<std::string, std::string> m_ModFileMap; // lowercase filename -> full path
        std::vector<ModFileInfo> m_ModList;
        size_t m_TotalRedirections = 0;
        mutable std::mutex m_Mutex;
    };
}
