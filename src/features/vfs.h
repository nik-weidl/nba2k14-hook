#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <cstdint>

namespace Features {
    struct ModFileInfo {
        std::string filename;      // e.g. "face1001.iff"
        std::string relativePath;  // e.g. "jerseys/face1001.iff"
        std::string fullPath;      // e.g. "S:\SteamLibrary\...\mods\jerseys\face1001.iff"
        std::string folderName;    // "" for loose root files, or "Folder/Subfolder"
        uint64_t fileSize;         // in bytes
        bool enabled = true;       // Per-mod toggle
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

        void SetModEnabled(size_t index, bool enabled);
        void SetFolderEnabled(const std::string& folderName, bool enabled);
        void EnableAllMods();
        void DisableAllMods();

        std::vector<std::string> GetFolders() const;
        bool IsFolderEnabled(const std::string& folderName) const;

        std::vector<ModFileInfo>& GetLoadedMods() { return m_ModList; }
        const std::string& GetModsDirectory() const { return m_ModsDirectory; }
        size_t GetModCount() const { return m_ModList.size(); }
        size_t GetEnabledModCount() const;
        size_t GetTotalRedirections() const { return m_TotalRedirections; }

        void SaveConfig();
        void LoadConfig();

    private:
        VFS() = default;
        void ScanDirectoryRecursive(const std::string& directory, const std::string& subDir);

        bool m_Enabled = true;
        std::string m_ModsDirectory;
        std::string m_ConfigFile;
        std::unordered_map<std::string, std::vector<size_t>> m_ModIndexMap; // lowercase filename -> list of indices
        std::vector<ModFileInfo> m_ModList;
        std::unordered_set<std::string> m_DisabledFilenames;
        size_t m_TotalRedirections = 0;
        mutable std::mutex m_Mutex;
    };
}
