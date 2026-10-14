#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <cstdint>

namespace Features {
    enum class ModCategory {
        Teams,              // Team assets (Floors, Stadiums, Jerseys, Logos, Dornas, Crowds)
        Cyberfaces,         // Player faces, hair, bodies (png####.iff, face####.iff)
        Presentation,       // Titlepages, Scoreboards, Replay wipes, Languages
        Shoes,              // Shoes & Gear
        GameplayAndBall,    // Basketball, animations
        Audio,              // Sounds, commentary
        Uncategorized       // Custom / Non-standard / Loose files
    };

    struct ModFileInfo {
        std::string filename;      // e.g. "face1001.iff"
        std::string relativePath;  // e.g. "jerseys/face1001.iff"
        std::string fullPath;      // e.g. "S:\SteamLibrary\...\mods\jerseys\face1001.iff"
        std::string folderName;    // "" for loose root files, or "Folder/Subfolder"
        uint64_t fileSize;         // in bytes
        bool enabled = true;       // Per-mod toggle

        // Classification metadata
        ModCategory category = ModCategory::Uncategorized;
        std::string categoryName;  // e.g. "Teams & Franchises", "Presentation & UI"
        std::string subGroupName;   // e.g. "Houston Rockets", "Title Screen & Menus"
        std::string assetTypeName; // e.g. "Floor / Court", "Title Screen"
        int teamId = -1;           // 0..33 or classic team ID or -1
    };

    struct CategoryGroup {
        std::string name;
        ModCategory category;
        std::vector<std::string> subGroupNames;
        std::unordered_map<std::string, std::vector<size_t>> subGroupIndices; // subGroupName -> mod index list
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
        void SetCategoryEnabled(ModCategory category, bool enabled);
        void SetSubGroupEnabled(ModCategory category, const std::string& subGroupName, bool enabled);
        void EnableAllMods();
        void DisableAllMods();

        std::vector<std::string> GetFolders() const;
        bool IsFolderEnabled(const std::string& folderName) const;
        bool IsCategoryEnabled(ModCategory category) const;
        bool IsSubGroupEnabled(ModCategory category, const std::string& subGroupName) const;

        std::vector<CategoryGroup> GetCategoryGroups() const;

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
        void ClassifyMod(ModFileInfo& info);

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
