@echo off
setlocal EnableDelayedExpansion

REM === Configuration ===
set BUILD_DIR=build-mingw
set GENERATOR="MinGW Makefiles"
set CI_MODE=%CI%

echo === Building Kuta ===

REM === Create build directory if it doesn't exist ===
if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
    echo Created build directory: %BUILD_DIR%
)

REM === Configure CMake options ===
set CMAKE_ARGS=-DCMAKE_BUILD_TYPE=Release
if "%CI_MODE%"=="true" (
    echo Running in CI mode
    set CMAKE_ARGS=%CMAKE_ARGS% -DENABLE_TESTING=ON
)

REM === Run CMake ===
cd /d "%BUILD_DIR%"
echo Configuring project with CMake...
cmake .. -G %GENERATOR% %CMAKE_ARGS%

if errorlevel 1 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

REM === Build with MinGW ===
echo Building project...
mingw32-make -j4

if errorlevel 1 (
    echo ERROR: Build failed
    exit /b 1
)

REM === Verify build ===
echo.
echo Verifying build output...
dir kuta.exe
dir *.spv 2>nul || echo Warning: No shader files found

echo.
echo ✅ Build complete. Executable: %CD%\kuta.exe
cd ..
endlocal
