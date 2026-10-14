#include "file_hook.h"
#include "../features/vfs.h"
#include "../core/logger.h"
#include <MinHook.h>

namespace Hooks {
    FileHook::CreateFileA_t FileHook::s_OriginalCreateFileA = nullptr;
    FileHook::CreateFileW_t FileHook::s_OriginalCreateFileW = nullptr;
    static void* s_TargetCreateFileA = nullptr;
    static void* s_TargetCreateFileW = nullptr;
    bool FileHook::s_Initialized = false;

    bool FileHook::Init() {
        LOG_INFO("Installing File I/O (CreateFile) hooks for VFS...");

        HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
        if (!hKernel32) {
            hKernel32 = LoadLibraryA("kernel32.dll");
        }

        s_TargetCreateFileA = (void*)GetProcAddress(hKernel32, "CreateFileA");
        s_TargetCreateFileW = (void*)GetProcAddress(hKernel32, "CreateFileW");

        if (!s_TargetCreateFileA) {
            LOG_ERROR("Failed to find CreateFileA in kernel32.dll!");
            return false;
        }

        if (MH_CreateHook(s_TargetCreateFileA, &Hooked_CreateFileA, reinterpret_cast<void**>(&s_OriginalCreateFileA)) != MH_OK) {
            LOG_ERROR("Failed to create hook for CreateFileA!");
            return false;
        }

        if (s_TargetCreateFileW) {
            MH_CreateHook(s_TargetCreateFileW, &Hooked_CreateFileW, reinterpret_cast<void**>(&s_OriginalCreateFileW));
        }

        if (MH_EnableHook(s_TargetCreateFileA) != MH_OK) {
            LOG_ERROR("Failed to enable CreateFileA hook!");
            return false;
        }

        if (s_TargetCreateFileW && s_OriginalCreateFileW) {
            MH_EnableHook(s_TargetCreateFileW);
        }

        s_Initialized = true;
        LOG_INFO("File I/O VFS hooks successfully installed!");
        return true;
    }

    void FileHook::Shutdown() {
        if (!s_Initialized) return;
        if (s_TargetCreateFileA) MH_DisableHook(s_TargetCreateFileA);
        if (s_TargetCreateFileW) MH_DisableHook(s_TargetCreateFileW);
        s_Initialized = false;
        LOG_INFO("File I/O hooks uninstalled.");
    }

    HANDLE WINAPI FileHook::Hooked_CreateFileA(
        LPCSTR lpFileName,
        DWORD dwDesiredAccess,
        DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes,
        HANDLE hTemplateFile
    ) {
        if (lpFileName && (dwDesiredAccess & GENERIC_READ)) {
            std::string redirected = Features::VFS::Get().GetRedirectPath(lpFileName);
            if (!redirected.empty()) {
                LOG_INFO("[VFS] REDIRECT: '%s' -> '%s'", lpFileName, redirected.c_str());
                return s_OriginalCreateFileA(
                    redirected.c_str(),
                    dwDesiredAccess,
                    dwShareMode,
                    lpSecurityAttributes,
                    dwCreationDisposition,
                    dwFlagsAndAttributes,
                    hTemplateFile
                );
            }
        }

        return s_OriginalCreateFileA(
            lpFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile
        );
    }

    HANDLE WINAPI FileHook::Hooked_CreateFileW(
        LPCWSTR lpFileName,
        DWORD dwDesiredAccess,
        DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes,
        HANDLE hTemplateFile
    ) {
        if (lpFileName && (dwDesiredAccess & GENERIC_READ)) {
            char ansiBuffer[MAX_PATH] = {};
            WideCharToMultiByte(CP_ACP, 0, lpFileName, -1, ansiBuffer, MAX_PATH, NULL, NULL);

            std::string redirected = Features::VFS::Get().GetRedirectPath(ansiBuffer);
            if (!redirected.empty()) {
                LOG_INFO("[VFS] REDIRECT (W): '%s' -> '%s'", ansiBuffer, redirected.c_str());
                wchar_t wideRedirected[MAX_PATH] = {};
                MultiByteToWideChar(CP_ACP, 0, redirected.c_str(), -1, wideRedirected, MAX_PATH);

                return s_OriginalCreateFileW(
                    wideRedirected,
                    dwDesiredAccess,
                    dwShareMode,
                    lpSecurityAttributes,
                    dwCreationDisposition,
                    dwFlagsAndAttributes,
                    hTemplateFile
                );
            }
        }

        return s_OriginalCreateFileW(
            lpFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile
        );
    }
}
