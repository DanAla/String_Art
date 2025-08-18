@echo off
REM String Art Generator Build Script
REM ALWAYS USE THIS TO BUILD - DO NOT USE DYNAMIC LINKING!
REM Static linking is REQUIRED to avoid crashes on Windows

echo ================== String Art Generator Build ==================
echo Building with STATIC linking (required for Windows)...
echo.

REM Clean old executables
if exist String_Art.exe (
    del String_Art.exe
    echo Cleaned old executable
)

REM Build with proper static linking
echo Compiling all source files with static linking...
g++ -std=c++17 -static-libgcc -static-libstdc++ -O2 -o String_Art.exe String_Art.cpp image_processing.cpp string_art_generator.cpp svg_generator.cpp

REM Check if build was successful
if exist String_Art.exe (
    echo.
    echo =================== BUILD SUCCESS! ===================
    echo String_Art.exe created successfully
    echo STATIC linking used - executable will not crash
    echo.
    echo Usage: String_Art.exe --help
    echo Test:  String_Art.exe -i image.png -n 400 -s 2000
    echo.
) else (
    echo.
    echo =================== BUILD FAILED! ===================
    echo Check for compilation errors above
    echo Make sure g++ is installed and in PATH
    echo.
    exit /b 1
)

echo Build completed at %date% %time%
echo.
