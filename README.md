# NBA 2K14 Native Hook & ImGui Overlay

[![Platform](https://img.shields.io/badge/Platform-Windows%20(x86)-blue.svg)](#)
[![API](https://img.shields.io/badge/DirectX-9%20%2F%209Ex-orange.svg)](#)
[![UI](https://img.shields.io/badge/GUI-Dear%20ImGui-green.svg)](#)
[![Detours](https://img.shields.io/badge/Hooks-MinHook-purple.svg)](#)
[![C++](https://img.shields.io/badge/Standard-C%2B%2B17-blueviolet.svg)](#)

A high-performance, non-destructive native C++ modding, feature injection, and runtime debugging framework for **NBA 2K14** (PC).

Built with an in-game graphical overlay using **Dear ImGui**, hardware-accelerated **DirectX 9 / 9Ex** hooks, and a proxy DLL architecture that requires **zero modification** to vanilla game files.

---

## Table of Contents

- [Overview & Architecture](#overview--architecture)
- [Key Features](#key-features)
- [In-Game Menu Interface](#in-game-menu-interface)
- [Reverse Engineering Foundation](#reverse-engineering-foundation)
- [Repository Structure](#repository-structure)
- [Prerequisites](#prerequisites)
- [Building from Source](#building-from-source)
- [Installation & Usage](#installation--usage)
- [Developer Guide (Adding Features)](#developer-guide-adding-features)
- [Diagnostics & Troubleshooting](#diagnostics--troubleshooting)
- [Credits & Third-Party Libraries](#credits--third-party-libraries)

---

## Overview & Architecture

Historically, modding NBA 2K14 required permanent, destructive overwriting of massive `.iff` archive files and manual binary patching of `nba2k14.exe`. 

This project introduces a modern, modular hook architecture:

```
┌────────────────────────────────────────────────────────┐
│                      nba2k14.exe                       │
└───────────────────────────┬────────────────────────────┘
                            │ (Imports DirectInput8)
                            ▼
┌────────────────────────────────────────────────────────┐
│              dinput8.dll (Native Hook)                 │
│                                                        │
│  ├── Proxy Forwarder ──► C:\Windows\System32\dinput8   │
│  ├── MinHook Engine                                    │
│  ├── D3D9 / D3D9Ex Hook (EndScene & Reset Detours)     │
│  ├── WndProc Input Trapper (<kbd>F8</kbd> / <kbd>Ins</kbd>) │
│  └── Dear ImGui Overlay Pipeline                       │
└───────────────────────────┬────────────────────────────┘
                            ▼
┌────────────────────────────────────────────────────────┐
│                   In-Game Overlay UI                   │
│   [Dashboard]  [Gameplay]  [Camera]  [Mods]  [About]   │
└────────────────────────────────────────────────────────┘
```

1. **Proxy DLL (`dinput8.dll`)**: When NBA 2K14 boots, the Windows PE loader loads `dinput8.dll` from the game directory. The hook exports `DirectInput8Create` and forwards all calls to the real system library at `C:\Windows\System32\dinput8.dll`. Gamepads, keyboards, and input devices operate completely normally.
2. **DirectX 9 Hooking (`Direct3DCreate9Ex`)**: Injects into the game's presentation loop via `IDirect3DDevice9::EndScene` and `IDirect3DDevice9::Reset`. Implemented using `Direct3DCreate9Ex` to ensure 100% compatibility with modern Windows 10/11 WDDM drivers and NVIDIA/AMD graphics architectures.
3. **Window Input Management (`WndProc`)**: Subclasses the game window message loop. When the menu is open, mouse and keyboard inputs are captured by Dear ImGui, preventing accidental in-game shots, passes, or pauses while interacting with UI sliders and buttons.

---

## Key Features

- **Virtual File System (VFS Mod Loader)**: Drop loose `.iff`, `.bin`, or audio files into the `mods/` folder. The hook intercepts file I/O at runtime and transparently redirects reads. Zero vanilla archives are ever touched or unpacked.
- **Per-Mod & Folder-Pack Toggles**: Enable or disable individual mod files or entire subfolder mod packs directly from the in-game menu with one click. Disabled mods seamlessly fallback to the original vanilla game archives.
- **Persistent Configuration**: Toggle states are saved automatically to `mods/disabled_mods.txt` and preserved across game restarts.
- **Non-Destructive**: Vanilla game files, archives, and executables remain completely untouched. To uninstall, simply delete `dinput8.dll`.
- **In-Game Graphical Overlay**: Press <kbd>F8</kbd> or <kbd>Insert</kbd> to open a sleek dark-mode menu with responsive tabs, sliders, toggles, and real-time telemetry.
- **Modern Hardware Compatibility**: Tested and verified on modern GPUs (e.g., NVIDIA GeForce RTX series). Explicitly isolates `System32\d3d9.dll` to prevent conflicts with legacy post-processing mods (such as ENBSeries or SweetFX).
- **Extensible Modding API**: Structured C++ feature manager designed for live memory manipulation, camera controls, player editing, and file diversion.

---

## In-Game Menu Interface

The overlay contains specialized tabs:

| Tab | Functionality |
| :--- | :--- |
| **Dashboard** | Real-time FPS telemetry, hook diagnostics, image base verification, and active subsystem indicators. |
| **Gameplay** | Real-time game speed multiplier slider, unlimited player stamina (fatigue lock), shot timing assist, and signature animation unlock switches. |
| **Camera & Visuals** | Field of View (FOV) adjustments (30°–120°), free camera mode toggle, and internal engine debug rendering. |
| **Mods (VFS)** | Virtual File System manager with live per-mod checkboxes, folder pack toggles, instant search filter, one-click folder opening, and config persistence. |
| **About** | Build metadata, versioning information, and credits. |

---

## Reverse Engineering Foundation

NBA 2K14 possesses architectural qualities that make it uniquely ideal for reverse engineering and native feature extension:

- **Fixed Virtual Memory Base (ASLR Disabled)**:
  - `DllCharacteristics: 0x8100` (`DYNAMIC_BASE: False`).
  - Fixed Image Base: `0x00400000`.
  - Memory offsets, global variables, and virtual method tables (vtables) are located at the exact same addresses on every run and across all machines.
- **Unencrypted Native Assembly**:
  - The `.text` code section (`VA: 0x00401000` – `0x00EC160A`) consists of uncompressed, raw 32-bit x86 compiled with MSVC 2010. There is no runtime obfuscation, anti-cheat, or DRM wrapper blocking hooks.
- **Complete C++ RTTI Preserved**:
  - Contains **1,313 intact Run-Time Type Information (RTTI) class descriptors**, exposing complete engine hierarchies:
    - `AI_SPECIAL_ABILITY_*` (Acrobat, Dimer, Posterizer, Deadeye, Lockdown Defender, etc.)
    - `GAMETYPE_*` (Base, 21, Dunk Contest)
    - `CAMERA_*` (25 distinct camera and presentation classes)
    - `AVVCUIDATABASE` / `GOOEY_*` (Visual Concepts' UI engine)
    - `AVVCFILE` / `AVVCWIN32FILEDEVICE` (File I/O subsystem)
- **137 Built-in Feature Flags (`VCFEATURECODE`)**:
  - The engine includes internal debug toggles (`VCFEATURECODE_ONLINE_DEBUG_ENABLE_TEXT`, `VCFEATURECODE_VCNETMARE_DEBUG_TEXT_ENABLE`, etc.) that can be manipulated in memory.

---

## Repository Structure

```
nba2k14-hook/
├── CMakeLists.txt                # CMake build script targeting 32-bit x86
├── dinput8.def                   # Module definition exporting DirectInput8Create
├── build.bat                     # Automated 1-click build & game deployment script
├── install_vs_buildtools.bat     # Automated installer for MSVC v143 & Windows SDK
├── README.md                     # Technical documentation & usage guide
├── .gitignore                    # Build artifacts and intermediate files filter
│
├── src/
│   ├── main.cpp                  # DllMain, thread launcher, and dinput8 proxy
│   ├── core/
│   │   ├── logger.h              # Thread-safe logging subsystem
│   │   └── logger.cpp            # Diagnostic file logging (nba2k14_hook.log)
│   ├── hooks/
│   │   ├── d3d9_hook.h           # Direct3D 9 / 9Ex device hook header
│   │   ├── d3d9_hook.cpp         # EndScene & Reset detours via MinHook
│   │   ├── wndproc_hook.h        # Window message procedure hook header
│   │   └── wndproc_hook.cpp      # Hotkey capture & ImGui mouse/keyboard routing
│   ├── features/
│   │   ├── feature_manager.h     # Feature settings registry & state models
│   │   └── feature_manager.cpp   # Per-frame gameplay logic execution
│   └── ui/
│       ├── menu.h                # ImGui menu controller header
│       └── menu.cpp              # Custom styled overlay interface
│
└── vendor/
    ├── imgui/                    # Dear ImGui (v1.90+) core & backends (Win32, DX9)
    └── minhook/                  # MinHook x86/x64 API hooking library
```

---

## Prerequisites

To build the 32-bit hook DLL from source:

1. **Windows 10 or 11** (64-bit OS running 32-bit target).
2. **Visual Studio 2022** (Community or Build Tools) with the **"Desktop development with C++"** workload:
   - MSVC v143 C++ x64/x86 build tools.
   - Windows 10 or 11 SDK.
3. **CMake** (v3.15 or newer).
4. *(Optional)* The repository includes [`install_vs_buildtools.bat`](file:///s:/SteamLibrary/steamapps/common/NBA2K14/nba2k14-hook/install_vs_buildtools.bat), which automates installing the MSVC toolchain if needed.

---

## Building from Source

### Option 1: One-Click Build Script (Recommended)
Run [`build.bat`](file:///s:/SteamLibrary/steamapps/common/NBA2K14/nba2k14-hook/build.bat) by double-clicking it or executing from CMD:

```cmd
build.bat
```

The script automatically detects CMake and Visual Studio, configures a 32-bit (Win32) build directory, compiles `dinput8.dll` in Release mode, and deploys it straight into your `NBA2K14/` game folder.

### Option 2: Command Line (Developer Command Prompt)
Open **x86 Native Tools Command Prompt for VS 2022** (or standard prompt with `vcvarsall.bat x86`):

```cmd
cd S:\SteamLibrary\steamapps\common\NBA2K14\nba2k14-hook
cmake -B build -A Win32
cmake --build build --config Release
```

The post-build step defined in [`CMakeLists.txt`](file:///s:/SteamLibrary/steamapps/common/NBA2K14/nba2k14-hook/CMakeLists.txt) automatically copies `build/Release/dinput8.dll` to `../dinput8.dll`.

---

## Installation & Usage

1. Verify that `dinput8.dll` is located in the game directory alongside `nba2k14.exe`:
   ```
   NBA2K14/
   ├── nba2k14.exe
   ├── dinput8.dll         <-- The compiled hook
   ├── nba2k14_hook.log    <-- Generated on startup
   └── ...
   ```
2. Launch **NBA 2K14** normally (via Steam or executable).
3. While in-game, press **<kbd>F8</kbd>** or **<kbd>Insert</kbd>** to toggle the overlay menu.
4. Interact with the menu using your mouse cursor. The game input is paused for the duration of menu interaction.
5. Press **<kbd>F8</kbd>** or click the close button `[X]` to return to gameplay.

---

## Developer Guide (Adding Features)

### Adding a New Setting / Feature Toggle
1. Open [`src/features/feature_manager.h`](file:///s:/SteamLibrary/steamapps/common/NBA2K14/nba2k14-hook/src/features/feature_manager.h) and declare your variable in `struct Settings`:
   ```cpp
   struct Settings {
       // ... existing settings
       bool myCustomFeature = false;
       float mySliderValue = 100.0f;
   };
   ```
2. Open [`src/ui/menu.cpp`](file:///s:/SteamLibrary/steamapps/common/NBA2K14/nba2k14-hook/src/ui/menu.cpp) and add the UI control under the desired tab:
   ```cpp
   ImGui::Checkbox("Enable My Custom Feature", &settings.myCustomFeature);
   if (settings.myCustomFeature) {
       ImGui::SliderFloat("Custom Value", &settings.mySliderValue, 0.0f, 200.0f, "%.1f");
   }
   ```
3. In [`src/features/feature_manager.cpp`](file:///s:/SteamLibrary/steamapps/common/NBA2K14/nba2k14-hook/src/features/feature_manager.cpp), apply the logic inside `Update()`:
   ```cpp
   void FeatureManager::Update() {
       if (m_Settings.myCustomFeature) {
           // Manipulate game memory directly at fixed addresses
       }
   }
   ```

### Hooking an Internal Game Function with MinHook
Because ASLR is disabled in `nba2k14.exe`, functions can be hooked at static virtual addresses:
```cpp
using GameFunction_t = void(__thiscall*)(void* thisPtr, int arg1);
GameFunction_t g_OriginalGameFunc = nullptr;

void __fastcall Hooked_GameFunction(void* thisPtr, void* edx, int arg1) {
    // Custom logic before game executes
    g_OriginalGameFunc(thisPtr, arg1);
    // Custom logic after game executes
}

// Inside your initialization routine:
void* targetAddress = reinterpret_cast<void*>(0x00XXXXXX);
MH_CreateHook(targetAddress, &Hooked_GameFunction, reinterpret_cast<void**>(&g_OriginalGameFunc));
MH_EnableHook(targetAddress);
```

---

## Diagnostics & Troubleshooting

The hook writes a diagnostic log file on startup at `NBA2K14/nba2k14_hook.log`.

Example healthy log output:
```text
=== NBA 2K14 Native Hook Log Started ===
[17:41:30] [INFO] DLL_PROCESS_ATTACH: Starting NBA 2K14 Native Hook...
[17:41:30] [INFO] NBA 2K14 Hook background thread started.
[17:41:30] [INFO] Loaded system dinput8.dll from C:\Windows\system32\dinput8.dll (proc: 0x748E33F0)
[17:41:31] [INFO] Initializing Feature Manager...
[17:41:31] [INFO] Initializing DirectX 9 hook via clean System32 runtime...
[17:41:31] [INFO] VTable obtained via Direct3DCreate9Ex: Reset=0x74A46680, EndScene=0x749C4950
[17:41:31] [INFO] DirectX 9 hooks successfully installed!
[17:41:31] [INFO] NBA 2K14 Hook initialization complete. Press [F8] in-game to toggle menu!
[17:41:32] [INFO] ImGui Menu successfully initialized.
[17:41:32] [INFO] WndProc hooked (Original: 0x009DC0A0)
[17:41:32] [INFO] DirectX 9 Game Overlay initialized on HWND: 0x00140726
```

### Common Issues
- **Menu does not appear when pressing <kbd>F8</kbd>**:
  - Try pressing <kbd>Insert</kbd>.
  - Verify that `nba2k14_hook.log` exists in the game folder and contains `DirectX 9 Game Overlay initialized`.
  - Ensure background overlays (e.g. GeForce Experience, RivaTuner, Discord) are not capturing exclusivity.
- **Game crashes on startup**:
  - Check `nba2k14_hook.log` to identify the last executed line.
  - Ensure no outdated 3rd-party `dinput8.dll` exists in the folder.
  - Make sure you compiled in **Release x86** (32-bit).

---

## Credits & Third-Party Libraries

- **[Dear ImGui](https://github.com/ocornut/imgui)** by Omar Cornut – Bloat-free graphical user interface library for C++.
- **[MinHook](https://github.com/TsudaKageyu/minhook)** by Tsuda Kageyu – The Minimalistic x86/x64 API Hooking Library for Windows.
- **DirectX 9 / 9Ex** by Microsoft Corporation.
- **Visual Concepts / 2K Sports** – Developers and publishers of NBA 2K14.

---

## License

This project is created for educational, research, and non-commercial modding purposes. NBA 2K14 is a registered trademark of Take-Two Interactive and 2K Sports.
