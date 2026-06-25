@echo off
setlocal
chcp 65001 >nul
cd /d "%~dp0"

if not exist "pcseg_gui.exe" (
    echo pcseg_gui.exe was not found.
    echo Please run package_windows.bat first, or copy this script into the executable directory.
    pause
    exit /b 1
)

if exist "data\scene.ply" (
    "%~dp0pcseg_gui.exe" "%~dp0data\scene.ply"
) else (
    "%~dp0pcseg_gui.exe"
)

if errorlevel 1 (
    echo.
    echo GUI failed to start. Please check whether this computer supports OpenGL.
    pause
    exit /b 1
)
