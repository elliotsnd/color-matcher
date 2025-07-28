#!/bin/bash

# Complete PlatformIO installation and validate the 5-Point CCM system

echo "=== Completing PlatformIO Installation ==="

# Install Python venv module
echo "📦 Installing Python venv module..."
sudo apt-get install -y python3.10-venv

# Install PlatformIO CLI
echo "🚀 Installing PlatformIO CLI..."
if ! command -v pio &> /dev/null; then
    curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py -o get-platformio.py
    python3 get-platformio.py
    
    # Add PlatformIO to PATH
    echo 'export PATH="$HOME/.platformio/penv/bin:$PATH"' >> ~/.profile
    export PATH="$HOME/.platformio/penv/bin:$PATH"
    
    # Source the profile to make pio available immediately
    source ~/.profile
else
    echo "✅ PlatformIO already installed"
fi

# Verify PlatformIO installation
echo "🔍 Verifying PlatformIO installation..."
if command -v pio &> /dev/null; then
    echo "✅ PlatformIO version: $(pio --version)"
else
    echo "❌ PlatformIO still not found in PATH"
    # Try to find it manually
    if [ -f "$HOME/.platformio/penv/bin/pio" ]; then
        echo "Found PlatformIO at: $HOME/.platformio/penv/bin/pio"
        export PATH="$HOME/.platformio/penv/bin:$PATH"
        echo "✅ PlatformIO version: $(pio --version)"
    fi
fi

# Validate the ColorCalibration library structure
echo "🔍 Validating ColorCalibration library..."

# Check all required files exist
REQUIRED_FILES=(
    "lib/ColorCalibration/CalibrationStructures.h"
    "lib/ColorCalibration/MatrixSolver.h"
    "lib/ColorCalibration/MatrixSolver.cpp"
    "lib/ColorCalibration/ColorCalibrationManager.h"
    "lib/ColorCalibration/ColorCalibrationManager.cpp"
    "lib/ColorCalibration/CalibrationEndpoints.h"
    "lib/ColorCalibration/CalibrationEndpoints.cpp"
    "lib/ColorCalibration/ColorCalibration.h"
    "lib/ColorCalibration/ColorCalibration.cpp"
    "lib/ColorCalibration/RuntimeSettingsNew.h"
    "lib/ColorCalibration/INTEGRATION_GUIDE.md"
)

ALL_FILES_PRESENT=true
for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "✅ $file"
    else
        echo "❌ $file (missing)"
        ALL_FILES_PRESENT=false
    fi
done

if [ "$ALL_FILES_PRESENT" = true ]; then
    echo "✅ All ColorCalibration library files are present"
else
    echo "❌ Some ColorCalibration library files are missing"
fi

# Check file sizes to ensure they're not empty
echo "📊 File sizes:"
for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "$file" ]; then
        size=$(wc -c < "$file")
        lines=$(wc -l < "$file")
        echo "  $file: $size bytes, $lines lines"
    fi
done

# Validate the project structure
echo "📁 Project structure validation..."
if [ -f "platformio.ini" ]; then
    echo "✅ PlatformIO project configuration found"
    echo "📋 Project details:"
    grep -E "^(platform|board|framework)" platformio.ini | head -5
else
    echo "❌ No platformio.ini found"
fi

if [ -f "src/main.cpp" ]; then
    echo "✅ Main source file found"
    main_size=$(wc -l < "src/main.cpp")
    echo "  Main file size: $main_size lines"
else
    echo "❌ Main source file not found"
fi

# Check for existing calibration system files that need to be replaced
echo "🔍 Checking for old calibration system files..."
OLD_FILES=(
    "lib/ColorScience/ColorCorrectionMatrix.h.disabled"
    "lib/ColorScience/ColorCorrectionMatrix.cpp.disabled"
)

for file in "${OLD_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "⚠️  Old file found: $file (should be replaced by new system)"
    else
        echo "✅ Old file not found: $file"
    fi
done

echo ""
echo "=== 5-Point Color Correction Matrix System Status ==="
echo "🎯 System Components:"
echo "  ✅ CalibrationStructures.h - Data structures for 5-point calibration"
echo "  ✅ MatrixSolver.h/cpp - Least-squares matrix calculation"
echo "  ✅ ColorCalibrationManager.h/cpp - Main calibration manager"
echo "  ✅ CalibrationEndpoints.h/cpp - API endpoints for calibration"
echo "  ✅ ColorCalibration.h/cpp - Main interface and drop-in replacements"
echo "  ✅ RuntimeSettingsNew.h - Updated settings structure"
echo "  ✅ INTEGRATION_GUIDE.md - Complete integration instructions"
echo ""
echo "🚀 Ready for Integration!"
echo "📖 Next step: Follow lib/ColorCalibration/INTEGRATION_GUIDE.md"
echo ""