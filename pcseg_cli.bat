@echo off
setlocal
chcp 65001 >nul
cd /d "%~dp0"

if exist "pcseg_cli.exe" (
    set "CLI_EXE=pcseg_cli.exe"
) else if exist "build\bin\pcseg_cli.exe" (
    set "CLI_EXE=build\bin\pcseg_cli.exe"
) else (
    echo pcseg_cli.exe was not found.
    echo Please run build.bat or package_windows.bat first.
    pause
    exit /b 1
)

echo Running demo segmentation...
"%CLI_EXE%" --demo -o "segmented_demo.ply"
if errorlevel 1 (
    echo.
    echo Demo failed.
    pause
    exit /b 1
)

echo.
echo Demo finished. Output file:
echo   segmented_demo.ply
pause
