@echo off
setlocal
chcp 65001 >nul
cd /d "%~dp0"

if not exist "pcseg_cli.exe" (
    echo pcseg_cli.exe was not found.
    echo Please run package_windows.bat first, or copy this script into the executable directory.
    pause
    exit /b 1
)

echo Running demo segmentation...
"%~dp0pcseg_cli.exe" --demo -o "%~dp0segmented_demo.ply"
if errorlevel 1 (
    echo.
    echo Demo failed.
    pause
    exit /b 1
)

echo.
echo Demo finished. Output file:
echo   %~dp0segmented_demo.ply
pause
