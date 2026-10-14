#include "vfs.h"
#include "../core/logger.h"
#include <windows.h>
#include <shellapi.h>
#include <fstream>
#include <algorithm>

namespace Features {
    VFS& VFS::Get() {
        static VFS instance;
        return instance;
    }

    static std::string ToLower(const std::string& str) {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower;
    }

    void VFS::Init() {
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string dir(exePath);
        size_t lastSlash = dir.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            dir = dir.substr(0, lastSlash);
        }

        m_ModsDirectory = dir + "\\mods";
        m_ConfigFile = m_ModsDirectory + "\\disabled_mods.txt";

        // Create mods directory if it doesn't exist
        DWORD attrib = GetFileAttributesA(m_ModsDirectory.c_str());
        if (attrib == INVALID_FILE_ATTRIBUTES || !(attrib & FILE_ATTRIBUTE_DIRECTORY)) {
            CreateDirectoryA(m_ModsDirectory.c_str(), NULL);
            LOG_INFO("[VFS] Created mods directory at: %s", m_ModsDirectory.c_str());
        }

        LoadConfig();
        Rescan();
    }

    void VFS::LoadConfig() {
        m_DisabledFilenames.clear();
        std::ifstream inFile(m_ConfigFile);
        if (!inFile.is_open()) return;

        std::string line;
        while (std::getline(inFile, line)) {
            // Trim whitespace
            size_t start = line.find_first_not_of(" \t\r\n");
            size_t end = line.find_last_not_of(" \t\r\n");
            if (start == std::string::npos) continue;
            std::string clean = line.substr(start, end - start + 1);

            if (clean.empty() || clean[0] == '#') continue;
            std::replace(clean.begin(), clean.end(), '\\', '/');
            m_DisabledFilenames.insert(ToLower(clean));
        }

        LOG_INFO("[VFS] Loaded %zu disabled mod entries from config.", m_DisabledFilenames.size());
    }

    void VFS::SaveConfig() {
        std::ofstream outFile(m_ConfigFile, std::ios::out | std::ios::trunc);
        if (!outFile.is_open()) {
            LOG_ERROR("[VFS] Failed to open %s for writing!", m_ConfigFile.c_str());
            return;
        }

        outFile << "# NBA 2K14 VFS Disabled Mods Configuration" << "\n";
        outFile << "# Files or relative paths listed here are disabled and will fallback to vanilla." << "\n\n";

        for (const auto& mod : m_ModList) {
            if (!mod.enabled) {
                outFile << ToLower(mod.relativePath) << "\n";
            }
        }
        outFile.flush();
    }

    static const std::unordered_map<int, std::string> s_TeamNames = {
        { 0, "Philadelphia 76ers" },
        { 1, "Charlotte Bobcats" },
        { 2, "Milwaukee Bucks" },
        { 3, "Chicago Bulls" },
        { 4, "Cleveland Cavaliers" },
        { 5, "Boston Celtics" },
        { 6, "LA Clippers" },
        { 7, "Memphis Grizzlies" },
        { 8, "Atlanta Hawks" },
        { 9, "Miami Heat" },
        { 10, "New Orleans Pelicans" },
        { 11, "Utah Jazz" },
        { 12, "Sacramento Kings" },
        { 13, "New York Knicks" },
        { 14, "Los Angeles Lakers" },
        { 15, "Orlando Magic" },
        { 16, "Dallas Mavericks" },
        { 17, "Brooklyn Nets" },
        { 18, "Denver Nuggets" },
        { 19, "Indiana Pacers" },
        { 20, "Detroit Pistons" },
        { 21, "Toronto Raptors" },
        { 22, "Houston Rockets" },
        { 23, "San Antonio Spurs" },
        { 24, "Phoenix Suns" },
        { 25, "Oklahoma City Thunder" },
        { 26, "Minnesota Timberwolves" },
        { 27, "Portland Trail Blazers" },
        { 28, "Golden State Warriors" },
        { 29, "Washington Wizards" },
        { 30, "East All-Stars" },
        { 31, "West All-Stars" },
        { 32, "Team Chuck" },
        { 33, "Team Shaq" },
        { 550, "'64-'65 Celtics" },
        { 551, "'64-'65 76ers" },
        { 552, "'70-'71 Bucks" },
        { 553, "'70-'71 Lakers" },
        { 554, "'70-'71 Hawks" },
        { 555, "'71-'72 Lakers" },
        { 556, "'71-'72 Knicks" },
        { 557, "'76-'77 76ers" },
        { 558, "'84-'85 76ers" },
        { 559, "'85-'86 Celtics" },
        { 560, "'85-'86 Bulls" },
        { 562, "'85-'86 Hawks" },
        { 564, "'88-'89 Pistons" },
        { 569, "'88-'89 Bulls" },
        { 570, "'89-'90 Cavaliers" },
        { 571, "'90-'91 Bulls" },
        { 572, "'90-'91 Lakers" },
        { 574, "'90-'91 Trail Blazers" },
        { 575, "'90-'91 Warriors" },
        { 576, "'92-'93 Bulls" },
        { 583, "'92-'93 Hornets" },
        { 585, "'93-'94 Bulls" },
        { 586, "'93-'94 Knicks" },
        { 588, "'93-'94 Nuggets" },
        { 589, "'93-'94 Rockets" },
        { 590, "'95-'96 Bulls" },
        { 591, "'95-'96 Supersonics" },
        { 592, "'97-'98 Bulls" },
        { 593, "'97-'98 Jazz" },
        { 594, "'97-'98 Lakers" },
        { 595, "'97-'98 Spurs" }
    };

    void VFS::ClassifyMod(ModFileInfo& info) {
        std::string fn = ToLower(info.filename);
        std::string folder = ToLower(info.folderName);

        auto getDigits = [](const std::string& str, size_t startPos, size_t& len) -> int {
            len = 0;
            while (startPos + len < str.length() && isdigit(str[startPos + len])) {
                len++;
            }
            if (len == 0) return -1;
            try {
                return std::stoi(str.substr(startPos, len));
            } catch (...) {
                return -1;
            }
        };

        // 1. Team Assets
        // Floor: f###.iff
        if (fn.rfind("f", 0) == 0 && fn.size() >= 5 && fn.rfind(".iff") == fn.size() - 4) {
            size_t dLen = 0;
            int id = getDigits(fn, 1, dLen);
            if (id >= 0 && 1 + dLen == fn.size() - 4) {
                info.category = ModCategory::Teams;
                info.categoryName = "Teams & Franchises";
                info.teamId = id;
                auto it = s_TeamNames.find(id);
                info.subGroupName = (it != s_TeamNames.end()) ? it->second : ("Team ID #" + std::to_string(id));
                info.assetTypeName = "Floor / Court";
                return;
            }
        }

        // Stadium: s###.iff
        if (fn.rfind("s", 0) == 0 && fn.size() >= 5 && fn.rfind(".iff") == fn.size() - 4) {
            size_t dLen = 0;
            int id = getDigits(fn, 1, dLen);
            if (id >= 0 && 1 + dLen == fn.size() - 4) {
                info.category = ModCategory::Teams;
                info.categoryName = "Teams & Franchises";
                info.teamId = id;
                auto it = s_TeamNames.find(id);
                info.subGroupName = (it != s_TeamNames.end()) ? it->second : ("Team ID #" + std::to_string(id));
                info.assetTypeName = "Stadium & Arena 3D";
                return;
            }
        }

        // Uniform / Jersey: uh###, ua###, ux###, r###, g###, l###
        if ((fn.rfind("uh", 0) == 0 || fn.rfind("ua", 0) == 0 || fn.rfind("ux", 0) == 0 ||
             fn.rfind("r", 0) == 0 || fn.rfind("g", 0) == 0 || fn.rfind("l", 0) == 0) &&
            fn.rfind(".iff") == fn.size() - 4) {
            size_t prefixLen = (fn[0] == 'u') ? 2 : 1;
            size_t dLen = 0;
            int id = getDigits(fn, prefixLen, dLen);
            if (id >= 0 && prefixLen + dLen == fn.size() - 4) {
                info.category = ModCategory::Teams;
                info.categoryName = "Teams & Franchises";
                info.teamId = id;
                auto it = s_TeamNames.find(id);
                info.subGroupName = (it != s_TeamNames.end()) ? it->second : ("Team ID #" + std::to_string(id));
                std::string uType = (fn.rfind("uh", 0) == 0) ? "Home Uniform" :
                                    (fn.rfind("ua", 0) == 0) ? "Away Uniform" :
                                    (fn.rfind("ux", 0) == 0) ? "Alternate Uniform" :
                                    (fn.rfind("r", 0) == 0) ? "Retro Uniform" : "Uniform";
                info.assetTypeName = uType;
                return;
            }
        }

        // Dornas / Ads: dorna###.iff
        if (fn.rfind("dorna", 0) == 0 && fn.rfind(".iff") == fn.size() - 4) {
            size_t dLen = 0;
            int id = getDigits(fn, 5, dLen);
            if (id >= 0) {
                info.category = ModCategory::Teams;
                info.categoryName = "Teams & Franchises";
                info.teamId = id;
                auto it = s_TeamNames.find(id);
                info.subGroupName = (it != s_TeamNames.end()) ? it->second : ("Team ID #" + std::to_string(id));
                info.assetTypeName = "Court Dornas / Ads";
                return;
            }
        }

        // Crowd: crowd###.iff
        if (fn.rfind("crowd", 0) == 0 && fn.rfind(".iff") == fn.size() - 4) {
            size_t dLen = 0;
            int id = getDigits(fn, 5, dLen);
            if (id >= 0) {
                info.category = ModCategory::Teams;
                info.categoryName = "Teams & Franchises";
                info.teamId = id;
                auto it = s_TeamNames.find(id);
                info.subGroupName = (it != s_TeamNames.end()) ? it->second : ("Team ID #" + std::to_string(id));
                info.assetTypeName = "Arena Crowd";
                return;
            }
        }

        // Logo: logo###.iff
        if (fn.rfind("logo", 0) == 0 && fn.rfind(".iff") == fn.size() - 4) {
            size_t dLen = 0;
            int id = getDigits(fn, 4, dLen);
            if (id >= 0) {
                info.category = ModCategory::Teams;
                info.categoryName = "Teams & Franchises";
                info.teamId = id;
                auto it = s_TeamNames.find(id);
                info.subGroupName = (it != s_TeamNames.end()) ? it->second : ("Team ID #" + std::to_string(id));
                info.assetTypeName = "Team Logo / Branding";
                return;
            }
        }

        // 2. Cyberfaces & Players
        // png####.iff
        if (fn.rfind("png", 0) == 0 && fn.rfind(".iff") == fn.size() - 4) {
            size_t dLen = 0;
            int id = getDigits(fn, 3, dLen);
            if (id >= 0 && 3 + dLen == fn.size() - 4) {
                info.category = ModCategory::Cyberfaces;
                info.categoryName = "Cyberfaces & Players";
                info.subGroupName = !info.folderName.empty() ? info.folderName : ("Player #" + std::to_string(id));
                info.assetTypeName = "3D Cyberface Model";
                return;
            }
        }

        // face####.iff
        if (fn.rfind("face", 0) == 0 && fn.rfind(".iff") == fn.size() - 4) {
            size_t dLen = 0;
            int id = getDigits(fn, 4, dLen);
            if (id >= 0 && 4 + dLen == fn.size() - 4) {
                info.category = ModCategory::Cyberfaces;
                info.categoryName = "Cyberfaces & Players";
                info.subGroupName = !info.folderName.empty() ? info.folderName : ("Player #" + std::to_string(id));
                info.assetTypeName = "Face Textures";
                return;
            }
        }

        // bar####.iff
        if (fn.rfind("bar", 0) == 0 && fn.rfind(".iff") == fn.size() - 4) {
            size_t dLen = 0;
            int id = getDigits(fn, 3, dLen);
            if (id >= 0 && 3 + dLen == fn.size() - 4) {
                info.category = ModCategory::Cyberfaces;
                info.categoryName = "Cyberfaces & Players";
                info.subGroupName = !info.folderName.empty() ? info.folderName : ("Player #" + std::to_string(id));
                info.assetTypeName = "Accessories / Arms";
                return;
            }
        }

        // 3. Presentation & UI
        if (fn == "titlepage.iff" || fn == "bootup.iff" || fn.find("wipe") != std::string::npos ||
            fn.find("scorebug") != std::string::npos || fn == "gamedata.iff" ||
            fn == "english.iff" || fn == "french.iff" || fn == "spanish.iff" || fn == "german.iff") {
            info.category = ModCategory::Presentation;
            info.categoryName = "Presentation & Broadcast UI";
            if (fn == "titlepage.iff" || fn == "bootup.iff") {
                info.subGroupName = "Title Screen & Menus";
                info.assetTypeName = (fn == "titlepage.iff") ? "Title Page" : "Bootup Screen";
            } else if (fn.find("scorebug") != std::string::npos || fn.find("wipe") != std::string::npos) {
                info.subGroupName = "Broadcast & Scoreboards";
                info.assetTypeName = (fn.find("wipe") != std::string::npos) ? "Replay Wipe Animation" : "Scoreboard";
            } else {
                info.subGroupName = "Text & Language Databases";
                info.assetTypeName = "Database File";
            }
            return;
        }

        // 4. Shoes & Gear
        if (fn.find("shoe") != std::string::npos || fn.find("sneaker") != std::string::npos) {
            info.category = ModCategory::Shoes;
            info.categoryName = "Shoes & Gear";
            info.subGroupName = !info.folderName.empty() ? info.folderName : "General Shoes";
            info.assetTypeName = "Shoe Asset";
            return;
        }

        // 5. Gameplay & Ball
        if (fn.find("ball") != std::string::npos || fn.find("anim") != std::string::npos || fn.find("dunk") != std::string::npos) {
            info.category = ModCategory::GameplayAndBall;
            info.categoryName = "Gameplay & Basketballs";
            info.subGroupName = (fn.find("ball") != std::string::npos) ? "Game Balls" : "Animations";
            info.assetTypeName = (fn.find("ball") != std::string::npos) ? "Basketball Asset" : "Animation Asset";
            return;
        }

        // 6. Audio
        if (fn.rfind(".bnk") == fn.size() - 4 || fn.rfind(".spc") == fn.size() - 4) {
            info.category = ModCategory::Audio;
            info.categoryName = "Audio & Sounds";
            info.subGroupName = !info.folderName.empty() ? info.folderName : "Soundbanks";
            info.assetTypeName = "Audio Bank";
            return;
        }

        // 7. Folder-derived Category
        if (!folder.empty()) {
            if (folder.find("team") != std::string::npos) {
                info.category = ModCategory::Teams;
                info.categoryName = "Teams & Franchises";
                info.subGroupName = info.folderName;
                info.assetTypeName = "Team Asset";
                return;
            } else if (folder.find("face") != std::string::npos || folder.find("player") != std::string::npos) {
                info.category = ModCategory::Cyberfaces;
                info.categoryName = "Cyberfaces & Players";
                info.subGroupName = info.folderName;
                info.assetTypeName = "Player Asset";
                return;
            } else if (folder.find("ui") != std::string::npos || folder.find("presentation") != std::string::npos || folder.find("menu") != std::string::npos) {
                info.category = ModCategory::Presentation;
                info.categoryName = "Presentation & Broadcast UI";
                info.subGroupName = info.folderName;
                info.assetTypeName = "UI Asset";
                return;
            }
        }

        // 8. Uncategorized (Ready for manual targeting)
        info.category = ModCategory::Uncategorized;
        info.categoryName = "Uncategorized Mods (Target Needed)";
        info.subGroupName = !info.folderName.empty() ? info.folderName : "Loose / Needs Target";
        info.assetTypeName = (fn.rfind("f", 0) == 0 && fn.find(".iff") != std::string::npos) ? "Court Mod (Needs Target)" : "Custom File";
    }

    void VFS::ScanDirectoryRecursive(const std::string& directory, const std::string& subDir) {
        std::string searchPath = directory + "\\*";
        WIN32_FIND_DATAA fd;
        HANDLE hFind = FindFirstFileA(searchPath.c_str(), &fd);

        if (hFind == INVALID_HANDLE_VALUE) return;

        do {
            if (strcmp(fd.cFileName, ".") == 0 || strcmp(fd.cFileName, "..") == 0) {
                continue;
            }

            // Ignore config file
            if (_stricmp(fd.cFileName, "disabled_mods.txt") == 0) {
                continue;
            }

            std::string fullPath = directory + "\\" + fd.cFileName;
            std::string relPath = subDir.empty() ? fd.cFileName : (subDir + "/" + fd.cFileName);

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                ScanDirectoryRecursive(fullPath, relPath);
            } else {
                std::string filenameLower = ToLower(fd.cFileName);
                std::string relPathLower = ToLower(relPath);
                std::string folderLower = ToLower(subDir);
                uint64_t fileSize = ((uint64_t)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;

                bool isEnabled = (m_DisabledFilenames.find(filenameLower) == m_DisabledFilenames.end()) &&
                                 (m_DisabledFilenames.find(relPathLower) == m_DisabledFilenames.end()) &&
                                 (subDir.empty() || m_DisabledFilenames.find(folderLower) == m_DisabledFilenames.end());

                size_t index = m_ModList.size();
                m_ModIndexMap[filenameLower].push_back(index);

                ModFileInfo info;
                info.filename = fd.cFileName;
                info.relativePath = relPath;
                info.fullPath = fullPath;
                info.folderName = subDir;
                info.fileSize = fileSize;
                info.enabled = isEnabled;

                ClassifyMod(info);

                m_ModList.push_back(info);

                LOG_INFO("[VFS] Registered mod file: %s -> [%s / %s / %s] (enabled: %s, size: %llu bytes)",
                    relPath.c_str(), info.categoryName.c_str(), info.subGroupName.c_str(), info.assetTypeName.c_str(),
                    isEnabled ? "YES" : "NO", fileSize);
            }
        } while (FindNextFileA(hFind, &fd));

        FindClose(hFind);
    }

    void VFS::Rescan() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_ModIndexMap.clear();
        m_ModList.clear();

        ScanDirectoryRecursive(m_ModsDirectory, "");

        LOG_INFO("[VFS] Rescan complete. Total mod files: %zu (Active: %zu)",
            m_ModList.size(), GetEnabledModCount());
    }

    void VFS::SetModEnabled(size_t index, bool enabled) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (index >= m_ModList.size()) return;

        m_ModList[index].enabled = enabled;
        std::string relLower = ToLower(m_ModList[index].relativePath);
        std::string fnLower = ToLower(m_ModList[index].filename);
        if (enabled) {
            m_DisabledFilenames.erase(relLower);
            m_DisabledFilenames.erase(fnLower);
        } else {
            m_DisabledFilenames.insert(relLower);
        }

        SaveConfig();
        LOG_INFO("[VFS] Mod '%s' is now %s.", m_ModList[index].relativePath.c_str(), enabled ? "ENABLED" : "DISABLED");
    }

    void VFS::SetFolderEnabled(const std::string& folderName, bool enabled) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        for (auto& mod : m_ModList) {
            if (mod.folderName == folderName) {
                mod.enabled = enabled;
                std::string relLower = ToLower(mod.relativePath);
                std::string fnLower = ToLower(mod.filename);
                if (enabled) {
                    m_DisabledFilenames.erase(relLower);
                    m_DisabledFilenames.erase(fnLower);
                } else {
                    m_DisabledFilenames.insert(relLower);
                }
            }
        }
        SaveConfig();
        LOG_INFO("[VFS] Folder '%s' is now %s.", folderName.c_str(), enabled ? "ENABLED" : "DISABLED");
    }

    std::vector<std::string> VFS::GetFolders() const {
        std::vector<std::string> folders;
        for (const auto& mod : m_ModList) {
            if (!mod.folderName.empty()) {
                if (std::find(folders.begin(), folders.end(), mod.folderName) == folders.end()) {
                    folders.push_back(mod.folderName);
                }
            }
        }
        return folders;
    }

    bool VFS::IsFolderEnabled(const std::string& folderName) const {
        for (const auto& mod : m_ModList) {
            if (mod.folderName == folderName && !mod.enabled) {
                return false;
            }
        }
        return true;
    }

    void VFS::SetCategoryEnabled(ModCategory category, bool enabled) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        for (auto& mod : m_ModList) {
            if (mod.category == category) {
                mod.enabled = enabled;
                std::string relLower = ToLower(mod.relativePath);
                std::string fnLower = ToLower(mod.filename);
                if (enabled) {
                    m_DisabledFilenames.erase(relLower);
                    m_DisabledFilenames.erase(fnLower);
                } else {
                    m_DisabledFilenames.insert(relLower);
                }
            }
        }
        SaveConfig();
        LOG_INFO("[VFS] Category %d is now %s.", (int)category, enabled ? "ENABLED" : "DISABLED");
    }

    void VFS::SetSubGroupEnabled(ModCategory category, const std::string& subGroupName, bool enabled) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        for (auto& mod : m_ModList) {
            if (mod.category == category && mod.subGroupName == subGroupName) {
                mod.enabled = enabled;
                std::string relLower = ToLower(mod.relativePath);
                std::string fnLower = ToLower(mod.filename);
                if (enabled) {
                    m_DisabledFilenames.erase(relLower);
                    m_DisabledFilenames.erase(fnLower);
                } else {
                    m_DisabledFilenames.insert(relLower);
                }
            }
        }
        SaveConfig();
        LOG_INFO("[VFS] Subgroup '%s' in category %d is now %s.", subGroupName.c_str(), (int)category, enabled ? "ENABLED" : "DISABLED");
    }

    bool VFS::IsCategoryEnabled(ModCategory category) const {
        for (const auto& mod : m_ModList) {
            if (mod.category == category && !mod.enabled) {
                return false;
            }
        }
        return true;
    }

    bool VFS::IsSubGroupEnabled(ModCategory category, const std::string& subGroupName) const {
        for (const auto& mod : m_ModList) {
            if (mod.category == category && mod.subGroupName == subGroupName && !mod.enabled) {
                return false;
            }
        }
        return true;
    }

    std::vector<CategoryGroup> VFS::GetCategoryGroups() const {
        std::lock_guard<std::mutex> lock(m_Mutex);

        std::vector<std::pair<ModCategory, std::string>> catOrder = {
            { ModCategory::Teams, "Teams & Franchises" },
            { ModCategory::Cyberfaces, "Cyberfaces & Players" },
            { ModCategory::Presentation, "Presentation & Broadcast UI" },
            { ModCategory::Shoes, "Shoes & Gear" },
            { ModCategory::GameplayAndBall, "Gameplay & Basketballs" },
            { ModCategory::Audio, "Audio & Sounds" },
            { ModCategory::Uncategorized, "Uncategorized Mods (Target Needed)" }
        };

        std::vector<CategoryGroup> result;

        for (const auto& catPair : catOrder) {
            CategoryGroup group;
            group.category = catPair.first;
            group.name = catPair.second;

            std::unordered_map<std::string, std::vector<size_t>> subMap;
            std::vector<std::string> subOrder;

            for (size_t i = 0; i < m_ModList.size(); i++) {
                if (m_ModList[i].category == catPair.first) {
                    const std::string& sg = m_ModList[i].subGroupName;
                    if (subMap.find(sg) == subMap.end()) {
                        subOrder.push_back(sg);
                    }
                    subMap[sg].push_back(i);
                }
            }

            if (!subOrder.empty()) {
                std::sort(subOrder.begin(), subOrder.end());
                group.subGroupNames = subOrder;
                group.subGroupIndices = subMap;
                result.push_back(group);
            }
        }

        return result;
    }

    void VFS::EnableAllMods() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_DisabledFilenames.clear();
        for (auto& mod : m_ModList) {
            mod.enabled = true;
        }
        SaveConfig();
        LOG_INFO("[VFS] Enabled all %zu mods.", m_ModList.size());
    }

    void VFS::DisableAllMods() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_DisabledFilenames.clear();
        for (auto& mod : m_ModList) {
            mod.enabled = false;
            m_DisabledFilenames.insert(ToLower(mod.relativePath));
        }
        SaveConfig();
        LOG_INFO("[VFS] Disabled all %zu mods.", m_ModList.size());
    }

    size_t VFS::GetEnabledModCount() const {
        size_t count = 0;
        for (const auto& mod : m_ModList) {
            if (mod.enabled) count++;
        }
        return count;
    }

    std::string VFS::GetRedirectPath(const char* originalPath) {
        if (!m_Enabled || !originalPath || originalPath[0] == '\0') {
            return "";
        }

        // Avoid infinite loop if path is already inside mods directory
        std::string pathStr(originalPath);
        std::string pathLower = ToLower(pathStr);
        if (pathLower.find("mods\\") != std::string::npos || pathLower.find("mods/") != std::string::npos) {
            return "";
        }

        // Extract base filename (e.g. "admiral_dunk_1.iff")
        size_t lastSlash = pathStr.find_last_of("\\/");
        std::string baseFilename = (lastSlash != std::string::npos) ? pathStr.substr(lastSlash + 1) : pathStr;
        std::string baseLower = ToLower(baseFilename);

        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_ModIndexMap.find(baseLower);
        if (it != m_ModIndexMap.end()) {
            for (size_t idx : it->second) {
                if (idx < m_ModList.size() && m_ModList[idx].enabled) {
                    m_TotalRedirections++;
                    return m_ModList[idx].fullPath;
                }
            }
        }

        return "";
    }

    void VFS::OpenFolderInExplorer() {
        ShellExecuteA(NULL, "open", m_ModsDirectory.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
}
