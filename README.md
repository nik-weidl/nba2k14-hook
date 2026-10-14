# NBA 2K14 Native Hook (DirectX 9 + Dear ImGui)

A high-performance, non-destructive native C++ modding and feature injection framework for **NBA 2K14**.

---

## Features

- **Non-Destructive DLL Proxy (`dinput8.dll`)**: Intercepts DirectInput 8 calls and proxies them to the real Windows system `dinput8.dll`. Vanilla game files and archives are never overwritten or altered.
- **In-Game ImGui Overlay**:
  - Toggled with <kbd>F8</kbd> or <kbd>Insert</kbd>.
  - Dark-mode responsive theme.
  - Interactive tabs:
    - **Dashboard**: Real-time FPS, hook diagnostics, and engine status.
    - **Gameplay**: Real-time speed scale, unlimited stamina, shot assist, and animation unlock toggles.
    - **Camera & Visuals**: FOV slider and free camera mode.
    - **Mods (VFS)**: Virtual File System loader status.
- **DirectX 9 Hooking**: Uses MinHook to detour `IDirect3DDevice9::EndScene` and `IDirect3DDevice9::Reset`.
- **Clean Input Routing**: Hooks `WndProc` to handle mouse and keyboard navigation in ImGui while blocking unwanted game actions when typing or clicking menu buttons.

---

## Project Structure

```
nba2k14-hook/
├── CMakeLists.txt        # CMake configuration targeting 32-bit x86
├── dinput8.def           # Module definition export forwarder
├── build.bat             # One-click build and auto-deploy script
├── src/
│   ├── main.cpp          # DllMain & DirectInput8Create proxy
│   ├── core/             # Logging and configuration
│   ├── hooks/            # Direct3D 9 & WndProc hooks
│   ├── features/         # Feature state management & gameplay tweaks
│   └── ui/               # Dear ImGui menu overlay
└── vendor/
    ├── imgui/            # Dear ImGui library & backends
    └── minhook/          # MinHook x86 detour library
```

---

## How to Build

### Prerequisites
- Visual Studio 2022 (with **Desktop development with C++** workload) or MSVC Build Tools.
- CMake (version 3.15 or newer).

### Building
Run `build.bat` or use standard CMake commands from a Developer Command Prompt:

```cmd
cd S:\SteamLibrary\steamapps\common\NBA2K14\nba2k14-hook
cmake -B build -A Win32
cmake --build build --config Release
```

The build script will automatically copy the resulting `dinput8.dll` into the main `NBA2K14/` game directory.

---

## Usage in Game
1. Launch NBA 2K14 as normal.
2. In-game, press <kbd>F8</kbd> or <kbd>Insert</kbd> to toggle the menu.
3. Check `nba2k14_hook.log` in the game folder for diagnostic messages and hook confirmation.
