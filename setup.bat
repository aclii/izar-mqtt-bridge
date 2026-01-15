@echo off
REM Quick Start Script for IZAR Water Meter MQTT Bridge (Windows)
setlocal enabledelayedexpansion

cd /d "%~dp0"

echo ================================================================
echo   IZAR Water Meter MQTT Bridge - Quick Setup
echo   M5 Stack Unit-C6L with SX1262 ^& Home Assistant
echo ================================================================
echo.

REM Check Python installation
python --version >nul 2>&1
if errorlevel 1 (
    echo [X] Python 3 is required but not installed.
    echo     Install Python 3.7 or later from https://www.python.org/downloads/
    echo     Make sure to check "Add Python to PATH" during installation.
    pause
    exit /b 1
)

echo [OK] Python 3 found
python --version
echo.

REM Check/Create Virtual Environment
set VENV_DIR=.venv
if not exist "%VENV_DIR%\" (
    echo Creating Python virtual environment...
    python -m venv %VENV_DIR%
    echo [OK] Virtual environment created at %VENV_DIR%
) else (
    echo [OK] Virtual environment found at %VENV_DIR%
)
echo.

REM Activate virtual environment
echo Activating virtual environment...
call %VENV_DIR%\Scripts\activate.bat

REM Upgrade pip
echo Upgrading pip...
python -m pip install --upgrade pip --quiet

REM Check for PlatformIO
where pio >nul 2>&1
if errorlevel 1 (
    echo Installing PlatformIO in virtual environment...
    pip install platformio
    echo [OK] PlatformIO installed
) else (
    echo [OK] PlatformIO found
    pio --version
)
echo.

REM Install/Update project dependencies
echo Installing project dependencies...
pio lib install
echo [OK] Dependencies installed
echo.

REM Configuration
echo Configuration:
echo    Edit the following files to customize:
echo    * include\config.h - WiFi, MQTT, and hardware pins
echo    * platformio.ini - Serial port settings
echo.

REM Build options
set /p BUILD_NOW="Do you want to build the project now? (y/n): "
if /i "%BUILD_NOW%"=="y" (
    echo.
    echo Building firmware...
    pio run -e m5stack-unit-c6l
    
    echo.
    set /p UPLOAD_NOW="Do you want to upload firmware now? (y/n): "
    if /i "!UPLOAD_NOW!"=="y" (
        echo.
        echo Uploading firmware...
        pio run -e m5stack-unit-c6l -t upload
        
        echo.
        set /p MONITOR_NOW="Do you want to monitor serial output? (y/n): "
        if /i "!MONITOR_NOW!"=="y" (
            pio device monitor -e m5stack-unit-c6l --baud 115200
        )
    )
)

echo.
echo ================================================================
echo   Setup Complete!
echo ================================================================
echo   Virtual Environment: %VENV_DIR%
echo.
echo   To activate the environment in future sessions:
echo     %VENV_DIR%\Scripts\activate.bat
echo.
echo   Next Steps:
echo   1. Edit include\config.h with your WiFi ^& MQTT settings
echo   2. Review README.md for wiring and setup
echo   3. Build: pio run -e m5stack-unit-c6l
echo   4. Upload: pio run -e m5stack-unit-c6l -t upload
echo   5. Monitor: pio device monitor -e m5stack-unit-c6l
echo.
echo   Documentation:
echo   * README.md - Single source of documentation
echo ================================================================
echo.

pause
