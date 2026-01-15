# Makefile for IZAR Water Meter MQTT Bridge Project
# Provides convenient shortcuts for common PlatformIO commands

.PHONY: help setup install build upload monitor clean all test list-devices

# Default target
help:
	@echo "IZAR Water Meter MQTT Bridge - Build Commands"
	@echo ""
	@echo "Available targets:"
	@echo "  make setup         - Create virtual environment and install PlatformIO"
	@echo "  make install       - Install project dependencies"
	@echo "  make build         - Build firmware"
	@echo "  make upload        - Upload firmware to device"
	@echo "  make monitor       - Monitor serial output"
	@echo "  make all           - Build and upload firmware"
	@echo "  make clean         - Clean build files"
	@echo "  make list-devices  - List connected USB devices"
	@echo "  make test          - Run tests (if available)"
	@echo ""
	@echo "Note: Make sure virtual environment is activated first:"
	@echo "  source .venv/bin/activate  (Linux/macOS)"
	@echo "  .venv\\Scripts\\activate     (Windows)"

# Create virtual environment and install PlatformIO
setup:
	@echo "Creating virtual environment..."
	python3 -m venv .venv
	@echo "Activating and installing PlatformIO..."
	@echo "Run: source .venv/bin/activate && pip install -r requirements.txt"

# Install project dependencies
install:
	@echo "Installing project dependencies..."
	pio lib install

# Build firmware
build:
	@echo "Building firmware for M5 Stack Unit-C6L..."
	pio run -e m5stack-unit-c6l

# Upload firmware to device
upload:
	@echo "Uploading firmware to device..."
	pio run -e m5stack-unit-c6l -t upload

# Monitor serial output
monitor:
	@echo "Starting serial monitor (115200 baud)..."
	@echo "Press Ctrl+C to exit"
	pio device monitor -e m5stack-unit-c6l --baud 115200

# Build and upload in one command
all: build upload
	@echo "Build and upload complete!"

# Clean build files
clean:
	@echo "Cleaning build files..."
	pio run -t clean
	@echo "Removing .pio directory..."
	rm -rf .pio

# List connected devices
list-devices:
	@echo "Connected USB devices:"
	pio device list

# Run tests (placeholder)
test:
	@echo "Running tests..."
	pio test -e m5stack-unit-c6l

# Build, upload, and monitor (full workflow)
flash: build upload monitor
