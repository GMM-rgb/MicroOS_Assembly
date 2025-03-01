@echo off
REM build.bat - Script to build MicroOS on Windows

REM Create build directory
if not exist build mkdir build
cd build

REM Check for required tools
where cmake >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Error: CMake is required but not installed.
    echo Please install CMake and try again.
    exit /b 1
)

REM Configure with CMake
echo Configuring MicroOS build with CMake...
cmake ..

REM Build the project
echo Building MicroOS...
cmake --build . --config Release

REM Report status
if %ERRORLEVEL% equ 0 (
    echo Build successful! The executable is in the build directory.
    echo Run the program with: microos.exe
) else (
    echo Build failed. Please check the error messages above.
)

cd ..