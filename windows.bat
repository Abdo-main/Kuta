@echo off
setlocal EnableDelayedExpansion

set BUILD_DIR=build-mingw
set GENERATOR="MinGW Makefiles"
set CI_MODE=%CI%

echo === Building Kuta ===

if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
    echo Created build directory: %BUILD_DIR%
)

set CMAKE_ARGS=-DCMAKE_BUILD_TYPE=Release
if "%CI_MODE%"=="true" (
    echo Running in CI mode
    set CMAKE_ARGS=%CMAKE_ARGS% -DENABLE_TESTING=ON
)

cd /d "%BUILD_DIR%"
echo Configuring project with CMake...
cmake .. -G %GENERATOR% %CMAKE_ARGS%

if errorlevel 1 (
    echo ERROR: CMake configuration failed
    cd ..
    exit /b 1
)

echo Building project...
mingw32-make -j4

if errorlevel 1 (
    echo ERROR: Build failed
    cd ..
    exit /b 1
)

echo.
echo Verifying build output...
if exist "kuta.dll" (
    echo Found: kuta.dll
) else (
    echo Warning: kuta.dll not found
)

if exist "libkuta.dll" (
    echo Found: libkuta.dll
) else (
    echo Warning: libkuta.dll not found
)

if exist "libkuta.dll.a" (
    echo Found: libkuta.dll.a (import library)
)

echo.
echo Build complete!
echo Library files should be in: %CD%
echo Examples should have library copied to their include/ directories
cd ..
endlocal
