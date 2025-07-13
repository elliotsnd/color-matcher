#!/bin/bash

# Color Matcher Build and Upload Script
# This script compiles and uploads the project with real-time settings support

echo "🚀 Building Color Matcher with Real-Time Settings..."
echo "=========================================="

# Check if platformio is installed
if ! command -v pio &> /dev/null; then
    echo "❌ PlatformIO CLI not found. Please install it first."
    exit 1
fi

# Clean previous build
echo "🧹 Cleaning previous build..."
pio run --target clean

# Build the project
echo "🔨 Building project..."
if pio run; then
    echo "✅ Build successful!"
else
    echo "❌ Build failed. Please check the errors above."
    exit 1
fi

# Ask user if they want to upload
read -p "📤 Upload to device? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "📤 Uploading to ESP32..."
    if pio run --target upload; then
        echo "✅ Upload successful!"
        echo ""
        echo "🎉 Real-time settings are now enabled!"
        echo "📱 Open the web interface to adjust settings live"
        echo "🌐 Default IP: 192.168.0.152"
        echo ""
        echo "🔧 Features added:"
        echo "   • LED brightness slider (real-time)"
        echo "   • Integration time selector (real-time)" 
        echo "   • IR compensation sliders (real-time)"
        echo "   • Color sampling controls"
        echo "   • Debug toggles"
        echo "   • Settings persistence"
    else
        echo "❌ Upload failed."
        exit 1
    fi
else
    echo "🏗️ Build completed. Upload skipped."
fi

echo ""
echo "📚 Usage:"
echo "   1. Connect to the device's web interface"
echo "   2. Scroll down to 'Real-Time Settings' panel"
echo "   3. Adjust sliders and dropdowns for immediate effect"
echo "   4. Use 'Load Current' to refresh from device"
echo "   5. Use 'Apply Settings' to save all changes"
echo "   6. Use 'Reset to Defaults' to restore factory settings"
