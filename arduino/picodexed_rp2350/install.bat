@echo off
REM ============================================================================
REM PicoDexed RP2350 - One-Click Windows Setup
REM ============================================================================
REM
REM This script downloads everything you need and puts it in the right place.
REM
REM HOW TO USE:
REM   1. Double-click this file (install.bat)
REM   2. If Windows asks "Do you want to allow this app", click "Yes"
REM   3. Wait for it to finish
REM   4. Open picodexed_rp2350.ino in Arduino IDE
REM
REM ============================================================================

echo.
echo ============================================================================
echo   PicoDexed RP2350 - Setup Script
echo ============================================================================
echo.

REM --- Check for git ---
where git >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo ERROR: Git is not installed!
    echo.
    echo Please install Git first:
    echo   1. Go to https://git-scm.com/download/win
    echo   2. Download and install it (click Next through everything)
    echo   3. Then run this script again
    echo.
    pause
    exit /b 1
)

REM --- Find the sketch directory (where this script is) ---
set SKETCH_DIR=%~dp0
echo Sketch directory: %SKETCH_DIR%
echo.

REM --- Download Synth_Dexed source files ---
echo [1/4] Downloading Synth_Dexed FM engine...
if exist "%SKETCH_DIR%dexed.h" (
    echo   Already downloaded, skipping.
) else (
    cd /d "%TEMP%"
    if exist Synth_Dexed rd /s /q Synth_Dexed
    git clone https://github.com/diyelectromusic/Synth_Dexed.git 2>nul
    if %ERRORLEVEL% neq 0 (
        echo   Trying alternate source...
        git clone https://github.com/dcoredump/Synth_Dexed.git 2>nul
    )
    if exist Synth_Dexed\src (
        echo   Copying source files to sketch...
        copy /Y "Synth_Dexed\src\*.h" "%SKETCH_DIR%" >nul
        copy /Y "Synth_Dexed\src\*.cpp" "%SKETCH_DIR%" >nul
        echo   Done!
    ) else (
        echo   ERROR: Could not download Synth_Dexed.
        echo   Please download manually from:
        echo     https://github.com/diyelectromusic/Synth_Dexed
        echo   And copy all files from the "src" folder into:
        echo     %SKETCH_DIR%
        echo.
    )
)

REM --- Find Arduino libraries folder ---
set LIBS_DIR=%USERPROFILE%\Documents\Arduino\libraries
if not exist "%LIBS_DIR%" (
    set LIBS_DIR=%USERPROFILE%\Arduino\libraries
)
if not exist "%LIBS_DIR%" (
    mkdir "%USERPROFILE%\Documents\Arduino\libraries"
    set LIBS_DIR=%USERPROFILE%\Documents\Arduino\libraries
)
echo.
echo Arduino libraries: %LIBS_DIR%

REM --- Download Arduino Audio Tools ---
echo.
echo [2/4] Downloading Arduino Audio Tools...
if exist "%LIBS_DIR%\arduino-audio-tools" (
    echo   Already installed, skipping.
) else (
    git clone https://github.com/pschatzmann/arduino-audio-tools.git "%LIBS_DIR%\arduino-audio-tools" 2>nul
    if %ERRORLEVEL% equ 0 (
        echo   Done!
    ) else (
        echo   ERROR: Could not download. Please install manually.
    )
)

REM --- Reminder for Library Manager libraries ---
echo.
echo [3/4] Library Manager libraries...
echo.
echo   You still need to install these in Arduino IDE:
echo   Go to Sketch ^> Include Library ^> Manage Libraries
echo   Search for and install each one:
echo.
echo     - Adafruit SSD1306      (click "Install All" when asked)
echo     - Adafruit GFX Library
echo     - Adafruit BusIO
echo     - Adafruit TinyUSB Library
echo.

REM --- Copy arm_math.h into place ---
echo [4/4] Setting up compatibility files...
if exist "%SKETCH_DIR%arm_math_compat.h" (
    copy /Y "%SKETCH_DIR%arm_math_compat.h" "%SKETCH_DIR%arm_math.h" >nul
    echo   arm_math.h created.
)

echo.
echo ============================================================================
echo   SETUP COMPLETE!
echo ============================================================================
echo.
echo   Next steps:
echo     1. Open picodexed_rp2350.ino in Arduino IDE
echo     2. Set these in the Tools menu:
echo        Board:             Raspberry Pi Pico 2
echo        CPU Architecture:  ARM Cortex-M33
echo        Flash Size:        4MB (Sketch: 1MB, FS: 3MB)
echo        USB Stack:         Adafruit TinyUSB
echo     3. Click Upload!
echo.
echo ============================================================================
echo.
pause
