@echo off
setlocal EnableDelayedExpansion

echo ============================================
echo Windows Setup Script
echo ============================================
echo.

set VCPKG_DIR=%CD%\vcpkg
set BUILD_DIR=build-mingw

if exist "%VCPKG_DIR%\vcpkg.exe" (
    echo vcpkg already installed at %VCPKG_DIR%
    goto :build_project
)

where git >nul 2>nul
if errorlevel 1 (
    echo ERROR: Git is not installed or not in PATH
    echo Please install Git from https://git-scm.com/downloads
    pause
    exit /b 1
)

echo.
echo [1/3] Cloning vcpkg...
git clone https://github.com/microsoft/vcpkg.git "%VCPKG_DIR%"
if errorlevel 1 (
    echo ERROR: Failed to clone vcpkg
    pause
    exit /b 1
)

echo.
echo [2/3] Bootstrapping vcpkg...
cd "%VCPKG_DIR%"
call bootstrap-vcpkg.bat
if errorlevel 1 (
    echo ERROR: Failed to bootstrap vcpkg
    cd "%~dp0"
    pause
    exit /b 1
)

cd "%~dp0"

:build_project
if not exist "vcpkg.json" (
    echo ERROR: vcpkg.json not found in project root
    echo Please create vcpkg.json with project dependencies
    pause
    exit /b 1
)

echo.
echo [3/3] Building project...
echo Note: CMake will automatically install dependencies from vcpkg.json
echo This may take 10-20 minutes on first run.
echo.

cmake -B "%BUILD_DIR%" -G "MinGW Makefiles" ^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_DIR%\scripts\buildsystems\vcpkg.cmake ^
    -DCMAKE_BUILD_TYPE=Release

if errorlevel 1 (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)

echo.
echo Building...
mingw32-make -C "%BUILD_DIR%" -j4

if errorlevel 1 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo ============================================
echo Build Complete!
echo ============================================
echo.
echo Library: %BUILD_DIR%\kuta.dll
echo.
echo To rebuild later, just run:
echo   mingw32-make -C %BUILD_DIR% -j4
echo.
echo To reconfigure:
echo   cmake -B %BUILD_DIR% -DCMAKE_TOOLCHAIN_FILE=%VCPKG_DIR%\scripts\buildsystems\vcpkg.cmake
echo.
pause
endlocal
