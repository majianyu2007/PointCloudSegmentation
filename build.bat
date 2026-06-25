@echo off
setlocal enabledelayedexpansion
chcp 65001 >nul

cd /d "%~dp0"

set "BUILD_DIR=build"
set "CONFIG=Release"
set "BUILD_GUI=ON"
set "RUN_TESTS=ON"
set "CLEAN=OFF"
set "GENERATOR="
set "CMAKE_EXE=cmake"
set "NINJA_EXE="
set "LOCAL_TOOLS=%~dp0tools"
set "TOOLCHAIN_LINK=%TEMP%\pcseg_toolchain_PointCloudSegmentation"

if exist "%LOCAL_TOOLS%\w64devkit\bin" (
    if exist "%TOOLCHAIN_LINK%\w64devkit" rmdir "%TOOLCHAIN_LINK%\w64devkit"
    if exist "%TOOLCHAIN_LINK%\cmake" rmdir "%TOOLCHAIN_LINK%\cmake"
    if exist "%TOOLCHAIN_LINK%\ninja" rmdir "%TOOLCHAIN_LINK%\ninja"
    if not exist "%TOOLCHAIN_LINK%" mkdir "%TOOLCHAIN_LINK%"
    mklink /J "%TOOLCHAIN_LINK%\w64devkit" "%LOCAL_TOOLS%\w64devkit" >nul
    if exist "%LOCAL_TOOLS%\cmake" mklink /J "%TOOLCHAIN_LINK%\cmake" "%LOCAL_TOOLS%\cmake" >nul
    if exist "%LOCAL_TOOLS%\ninja" mklink /J "%TOOLCHAIN_LINK%\ninja" "%LOCAL_TOOLS%\ninja" >nul
    set "LOCAL_TOOLS=%TOOLCHAIN_LINK%"
)

if exist "%LOCAL_TOOLS%\w64devkit\bin" set "PATH=%LOCAL_TOOLS%\w64devkit\bin;%PATH%"
if exist "%LOCAL_TOOLS%\cmake\bin" set "PATH=%LOCAL_TOOLS%\cmake\bin;%PATH%"
if exist "%LOCAL_TOOLS%\ninja" set "PATH=%LOCAL_TOOLS%\ninja;%PATH%"
if exist "%LOCAL_TOOLS%\w64devkit\bin" (
    set "GCC_EXEC_PREFIX=%LOCAL_TOOLS%\w64devkit\lib\gcc\"
    set "COMPILER_PATH=%LOCAL_TOOLS%\w64devkit\libexec\gcc\x86_64-w64-mingw32\15.2.0;%LOCAL_TOOLS%\w64devkit\bin"
    set "LIBRARY_PATH=%LOCAL_TOOLS%\w64devkit\lib\gcc\x86_64-w64-mingw32\15.2.0;%LOCAL_TOOLS%\w64devkit\lib"
    set "CFLAGS=-idirafter %LOCAL_TOOLS%\w64devkit\include"
    set "CXXFLAGS=-idirafter %LOCAL_TOOLS%\w64devkit\include"
)

:parse
if "%~1"=="" goto parsed
if /I "%~1"=="--debug" set "CONFIG=Debug"
if /I "%~1"=="--no-gui" set "BUILD_GUI=OFF"
if /I "%~1"=="--skip-tests" set "RUN_TESTS=OFF"
if /I "%~1"=="--clean" set "CLEAN=ON"
if /I "%~1"=="--ninja" set "GENERATOR=-G Ninja"
if /I "%~1"=="--help" goto help
shift
goto parse

:parsed
if "%CLEAN%"=="ON" (
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)

where cmake >nul 2>nul
if errorlevel 1 (
    if exist "%LOCAL_TOOLS%\w64devkit\bin\cmake.exe" set "CMAKE_EXE=%LOCAL_TOOLS%\w64devkit\bin\cmake.exe"
    if exist "%LOCAL_TOOLS%\cmake\bin\cmake.exe" set "CMAKE_EXE=%LOCAL_TOOLS%\cmake\bin\cmake.exe"
    if exist "%ProgramFiles%\CMake\bin\cmake.exe" set "CMAKE_EXE=%ProgramFiles%\CMake\bin\cmake.exe"
    if exist "%ProgramFiles%\JetBrains\CLion 2025.2\bin\cmake\win\x64\bin\cmake.exe" set "CMAKE_EXE=%ProgramFiles%\JetBrains\CLion 2025.2\bin\cmake\win\x64\bin\cmake.exe"
    if exist "%ProgramFiles%\CodeBlocks\MinGW\bin\cmake.exe" set "CMAKE_EXE=%ProgramFiles%\CodeBlocks\MinGW\bin\cmake.exe"
)

if "%CMAKE_EXE%"=="cmake" (
    where cmake >nul 2>nul
    if errorlevel 1 (
        echo CMake was not found. Please install CMake 3.16+ or add it to PATH.
        exit /b 1
    )
)

where ninja >nul 2>nul
if not errorlevel 1 set "NINJA_EXE=ninja"
if "%NINJA_EXE%"=="" (
    if exist "%LOCAL_TOOLS%\w64devkit\bin\ninja.exe" set "NINJA_EXE=%LOCAL_TOOLS%\w64devkit\bin\ninja.exe"
    if exist "%LOCAL_TOOLS%\ninja\ninja.exe" set "NINJA_EXE=%LOCAL_TOOLS%\ninja\ninja.exe"
    if exist "%ProgramFiles%\JetBrains\CLion 2025.2\bin\ninja\win\x64\ninja.exe" set "NINJA_EXE=%ProgramFiles%\JetBrains\CLion 2025.2\bin\ninja\win\x64\ninja.exe"
    if exist "%ProgramFiles%\CodeBlocks\MinGW\bin\ninja.exe" set "NINJA_EXE=%ProgramFiles%\CodeBlocks\MinGW\bin\ninja.exe"
)

if "%GENERATOR%"=="" (
    if not "%NINJA_EXE%"=="" set "GENERATOR=-G Ninja"
)

echo [1/3] Configure
if "%GENERATOR%"=="-G Ninja" (
    "%CMAKE_EXE%" -S . -B "%BUILD_DIR%" %GENERATOR% "-DCMAKE_MAKE_PROGRAM:FILEPATH=%NINJA_EXE%" -DCMAKE_BUILD_TYPE=%CONFIG% -DPCSEG_BUILD_GUI=%BUILD_GUI%
) else (
    "%CMAKE_EXE%" -S . -B "%BUILD_DIR%" %GENERATOR% -DCMAKE_BUILD_TYPE=%CONFIG% -DPCSEG_BUILD_GUI=%BUILD_GUI%
)
if errorlevel 1 goto fail

echo [2/3] Build
"%CMAKE_EXE%" --build "%BUILD_DIR%" --config %CONFIG%
if errorlevel 1 goto fail

if "%RUN_TESTS%"=="ON" (
    echo [3/3] Test
    "%CMAKE_EXE%" --build "%BUILD_DIR%" --target test --config %CONFIG%
    if errorlevel 1 goto fail
) else (
    echo [3/3] Test skipped
)

echo.
echo Build finished.
echo Command line: "%BUILD_DIR%\bin\pcseg_cli.exe" --demo -o "%BUILD_DIR%\segmented_demo.ply"
if "%BUILD_GUI%"=="ON" echo GUI:          "%BUILD_DIR%\bin\pcseg_gui.exe"
exit /b 0

:help
echo Usage: build.bat [--clean] [--debug] [--no-gui] [--skip-tests] [--ninja]
echo   --clean       Remove build directory before configuring.
echo   --debug       Build Debug instead of Release.
echo   --no-gui      Build core, CLI and tests only.
echo   --skip-tests  Do not run CTest after building.
echo   --ninja       Force Ninja generator.
exit /b 0

:fail
echo.
echo Build failed. If GLFW download is blocked, run: build.bat --no-gui
echo If the project path contains Chinese characters, run build.bat --clean after installing Ninja.
exit /b 1
