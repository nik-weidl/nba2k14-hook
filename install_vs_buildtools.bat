@echo off
:: Auto-request administrator privileges
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo [INFO] Requesting Administrator privileges to run the Visual Studio installer...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

echo ====================================================================
echo   Installing Visual Studio 2022 C++ Build Tools
echo   Workload: Microsoft.VisualStudio.Workload.VCTools (MSVC + Win SDK)
echo ====================================================================
echo.

set INSTALLER="C:\Users\Niklas\AppData\Local\Temp\WinGet\Microsoft.VisualStudio.2022.BuildTools.17.14.41\vs_BuildTools.exe"

if not exist %INSTALLER% (
    echo [INFO] Downloading vs_BuildTools.exe...
    curl -L "https://aka.ms/vs/17/release/vs_BuildTools.exe" -o "%TEMP%\vs_BuildTools.exe"
    set INSTALLER="%TEMP%\vs_BuildTools.exe"
)

echo [INFO] Launching installer...
%INSTALLER% --passive --wait --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended

echo.
echo ====================================================================
echo   Installation finished! You can now run build.bat to compile the hook.
echo ====================================================================
pause
