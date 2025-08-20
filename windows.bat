@echo off
setlocal

REM === Configuration ===
set PROJECT_ROOT=G:\Dev\Kuta-main
set BUILD_DIR=%PROJECT_ROOT%\build-mingw
set GENERATOR="MinGW Makefiles"

REM === Create build directory if it doesn't exist ===
if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
    echo Created build directory: %BUILD_DIR%
) else (
    echo Using existing build directory: %BUILD_DIR%
)

REM === Run CMake ===
cd /d "%BUILD_DIR%"
echo Configuring project with CMake...
cmake .. -G %GENERATOR%

REM === Build with MinGW ===
echo Building project...
mingw32-make

echo.
echo ✅ Build complete. Executable should be in: %BUILD_DIR%
endlocal
