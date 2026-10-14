@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo   NBA 2K14 Native Hook - Build Script (32-bit x86)
echo ========================================================

:: Check CMake in PATH or standard location
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    if exist "C:\Program Files\CMake\bin\cmake.exe" (
        set "PATH=C:\Program Files\CMake\bin;%PATH%"
    ) else (
        echo [ERROR] CMake is not found!
        pause
        exit /b 1
    )
)

:: Find MSVC vcvarsall.bat
set VCVARS="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
if not exist %VCVARS% (
    set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if exist !VSWHERE! (
        for /f "usebackq tokens=*" %%i in (`!VSWHERE! -latest -requires Microsoft.Component.MSBuild -property installationPath`) do (
            if exist "%%i\VC\Auxiliary\Build\vcvarsall.bat" (
                set VCVARS="%%i\VC\Auxiliary\Build\vcvarsall.bat"
            )
        )
    )
)

if exist %VCVARS% (
    echo [INFO] Initializing MSVC x86 environment...
    call %VCVARS% x86
) else (
    echo [ERROR] Could not find vcvarsall.bat!
    pause
    exit /b 1
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
