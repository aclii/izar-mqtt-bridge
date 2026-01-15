#!/bin/bash
# Quick Start Script for IZAR Water Meter MQTT Bridge

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_DIR"

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  IZAR Water Meter MQTT Bridge - Quick Setup                    ║"
echo "║  M5 Stack Unit-C6L with SX1262 & Home Assistant                ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Check Python installation
if ! command -v python3 &> /dev/null; then
    echo "❌ Python 3 is required but not installed."
    echo "   Install Python 3.7 or later and try again."
    exit 1
fi

echo "✓ Python 3 found: $(python3 --version)"
echo ""

# Check/Create Virtual Environment
VENV_DIR=".venv"
if [ ! -d "$VENV_DIR" ]; then
    echo "📦 Creating Python virtual environment..."
    python3 -m venv "$VENV_DIR"
    echo "✓ Virtual environment created at $VENV_DIR"
else
    echo "✓ Virtual environment found at $VENV_DIR"
fi
echo ""

# Activate virtual environment
echo "🔌 Activating virtual environment..."
source "$VENV_DIR/bin/activate"

# Upgrade pip
echo "⬆️  Upgrading pip..."
pip install --upgrade pip --quiet

# Check for PlatformIO in virtual environment
if ! command -v pio &> /dev/null; then
    echo "⚙️  Installing PlatformIO in virtual environment..."
    pip install platformio
    echo "✓ PlatformIO installed"
else
    echo "✓ PlatformIO found: $(pio --version)"
fi
echo ""

# Install/Update project dependencies
echo "📚 Installing project dependencies..."
pio lib install
echo "✓ Dependencies installed"
echo ""

# Configuration
echo "📝 Configuration:"
echo "   Edit the following files to customize:"
echo "   • include/config.h - WiFi, MQTT, and hardware pins"
echo "   • platformio.ini - Serial port settings"
echo ""

# Build options
read -p "Do you want to build the project now? (y/n) " -n 1 -r
echo ""

if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo ""
    echo "🔨 Building firmware..."
    pio run -e m5stack-unit-c6l
    
    echo ""
    read -p "Do you want to upload firmware now? (y/n) " -n 1 -r
    echo ""
    
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo ""
        echo "📤 Uploading firmware..."
        pio run -e m5stack-unit-c6l -t upload
        
        echo ""
        read -p "Do you want to monitor serial output? (y/n) " -n 1 -r
        echo ""
        
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            pio device monitor -e m5stack-unit-c6l --baud 115200
        fi
    fi
fi

echo ""
echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  Setup Complete!                                               ║"
echo "╠════════════════════════════════════════════════════════════════╣"
echo "║  Virtual Environment: $VENV_DIR"
echo "║  "
echo "║  To activate the environment in future sessions:              ║"
echo "║    source $VENV_DIR/bin/activate  (Linux/macOS)"
echo "║    $VENV_DIR\\Scripts\\activate     (Windows)"
echo "║                                                                ║"
echo "║  Next Steps:                                                   ║"
echo "║  1. Edit include/config.h with your WiFi & MQTT settings      ║"
echo "║  2. Review README.md for wiring and setup                     ║"
echo "║  3. Build: pio run -e m5stack-unit-c6l                        ║"
echo "║  4. Upload: pio run -e m5stack-unit-c6l -t upload             ║"
echo "║  5. Monitor: pio device monitor -e m5stack-unit-c6l          ║"
echo "║                                                                ║"
echo "║  Documentation:                                                ║"
echo "║  • README.md - Single source of documentation                 ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""
