@echo off
setlocal
chcp 65001 >nul
cd /d "%~dp0"

if exist "pcseg_gui.exe" (
    set "GUI_EXE=%~dp0pcseg_gui.exe"
) else if exist "build\bin\pcseg_gui.exe" (
    set "GUI_EXE=%~dp0build\bin\pcseg_gui.exe"
) else (
    echo pcseg_gui.exe was not found.
    echo Please run build.bat or package_windows.bat first.
    pause
    exit /b 1
)

if exist "%~dp0data\scene.ply" (
    "%GUI_EXE%" "%~dp0data\scene.ply"
) else if exist "%~dp0build\bin\data\scene.ply" (
    "%GUI_EXE%" "%~dp0build\bin\data\scene.ply"
) else (
    "%GUI_EXE%"
)

if errorlevel 1 (
    echo.
    echo GUI failed to start. Please check whether this computer supports OpenGL.
    pause
    exit /b 1
)
