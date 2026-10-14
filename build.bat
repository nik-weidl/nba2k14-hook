@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo   NBA 2K14 Native Hook - Build Script (32-bit x86)
echo ========================================================

:: Check if CMake is installed
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake is not found in PATH!
    echo Please install CMake or run from Developer Command Prompt.
    pause
    exit /b 1
)

:: Try to find vswhere
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist %VSWHERE% (
    for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -requires Microsoft.Component.MSBuild -property installationPath`) do (
        set VS_PATH=%%i
    )
)

if defined VS_PATH (
    if exist "!VS_PATH!\VC\Auxiliary\Build\vcvarsall.bat" (
        echo [INFO] Setting up Visual Studio x86 environment from: !VS_PATH!
        call "!VS_PATH!\VC\Auxiliary\Build\vcvarsall.bat" x86
    )
)

:: Create build folder
if not exist build (
    mkdir build
)

echo [INFO] Configuring CMake project (Win32 / x86)...
cmake -B build -A Win32
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed!
    pause
    exit /b 1
)

echo [INFO] Building Release dinput8.dll...
cmake --build build --config Release
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================================
echo   BUILD SUCCEEDED!
echo   dinput8.dll has been deployed to NBA 2K14 game directory.
echo   Press [F8] or [Insert] in-game to open the menu.
echo ========================================================
pause
