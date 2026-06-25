@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul

cd /d "%~dp0"

set "DIST_DIR=dist\PointCloudSegmentation_Run"

call build.bat --clean --skip-tests
if errorlevel 1 goto fail

if exist "%DIST_DIR%" rmdir /s /q "%DIST_DIR%"
mkdir "%DIST_DIR%"
mkdir "%DIST_DIR%\data"

copy /y "build\bin\pcseg_cli.exe" "%DIST_DIR%\" >nul
copy /y "build\bin\pcseg_gui.exe" "%DIST_DIR%\" >nul
copy /y "readme.txt" "%DIST_DIR%\" >nul
copy /y "README.md" "%DIST_DIR%\" >nul
xcopy "data" "%DIST_DIR%\data" /e /i /y >nul

copy /y "run_demo.bat" "%DIST_DIR%\" >nul
copy /y "run_gui.bat" "%DIST_DIR%\" >nul

echo.
echo Package is ready:
echo   %DIST_DIR%
echo.
echo Teacher can run:
echo   %DIST_DIR%\run_gui.bat
echo   %DIST_DIR%\run_demo.bat
exit /b 0

:fail
echo.
echo Package failed. Try building without GUI: build.bat --clean --no-gui
exit /b 1
