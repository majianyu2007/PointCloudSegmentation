@echo off
setlocal
chcp 65001 >nul
cd /d "%~dp0"

if exist "pcseg_cli.exe" (
    set "CLI_EXE=%~dp0pcseg_cli.exe"
) else if exist "build\bin\pcseg_cli.exe" (
    set "CLI_EXE=%~dp0build\bin\pcseg_cli.exe"
) else (
    echo pcseg_cli.exe was not found.
    echo Please run build.bat or package_windows.bat first.
    pause
    exit /b 1
)

echo Running demo segmentation...
"%CLI_EXE%" --demo -o "%~dp0segmented_demo.ply"
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
