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
                m_ModList.push_back({ fd.cFileName, relPath, fullPath, subDir, fileSize, isEnabled });

                LOG_INFO("[VFS] Registered mod file: %s (folder: '%s', enabled: %s, size: %llu bytes)",
                    relPath.c_str(), subDir.c_str(), isEnabled ? "YES" : "NO", fileSize);
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
