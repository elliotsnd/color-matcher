#!/bin/bash

echo "ESP32 Color Sensor - Optimized Upload"
echo "====================================="
echo

# Check if Python is available
if ! command -v python3 &> /dev/null; then
    if ! command -v python &> /dev/null; then
        echo "Error: Python not found. Please install Python first."
        exit 1
    else
        PYTHON_CMD="python"
    fi
else
    PYTHON_CMD="python3"
fi

# Make script executable
chmod +x upload_optimized.py

# Run the optimized upload script
$PYTHON_CMD upload_optimized.py

echo
echo "Upload process completed."
