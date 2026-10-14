#include "vfs.h"
#include "../core/logger.h"
#include <windows.h>
#include <shellapi.h>
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

        // Create mods directory if it doesn't exist
        DWORD attrib = GetFileAttributesA(m_ModsDirectory.c_str());
        if (attrib == INVALID_FILE_ATTRIBUTES || !(attrib & FILE_ATTRIBUTE_DIRECTORY)) {
            CreateDirectoryA(m_ModsDirectory.c_str(), NULL);
            LOG_INFO("[VFS] Created mods directory at: %s", m_ModsDirectory.c_str());
        }

        Rescan();
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

            std::string fullPath = directory + "\\" + fd.cFileName;
            std::string relPath = subDir.empty() ? fd.cFileName : (subDir + "/" + fd.cFileName);

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                ScanDirectoryRecursive(fullPath, relPath);
            } else {
                std::string filenameLower = ToLower(fd.cFileName);
                uint64_t fileSize = ((uint64_t)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;

                m_ModFileMap[filenameLower] = fullPath;
                m_ModList.push_back({ fd.cFileName, relPath, fullPath, fileSize });

                LOG_INFO("[VFS] Registered mod file: %s (size: %llu bytes)", relPath.c_str(), fileSize);
            }
        } while (FindNextFileA(hFind, &fd));

        FindClose(hFind);
    }

    void VFS::Rescan() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_ModFileMap.clear();
        m_ModList.clear();

        ScanDirectoryRecursive(m_ModsDirectory, "");

        LOG_INFO("[VFS] Rescan complete. Total active mod files: %zu", m_ModList.size());
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
        auto it = m_ModFileMap.find(baseLower);
        if (it != m_ModFileMap.end()) {
            m_TotalRedirections++;
            return it->second;
        }

        return "";
    }

    void VFS::OpenFolderInExplorer() {
        ShellExecuteA(NULL, "open", m_ModsDirectory.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
}
