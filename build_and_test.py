#!/usr/bin/env python3
"""
Build and Test Script for ESP32-S3 Color Sensor with Storage Optimizations
"""

import os
import subprocess
import sys
import json
import time

def run_command(command, description, capture_output=True):
    """Run a command and handle errors"""
    print(f"🔧 {description}...")
    try:
        result = subprocess.run(command, shell=True, capture_output=capture_output, text=True)
        if result.returncode == 0:
            print(f"✅ {description} completed successfully")
            return result.stdout
        else:
            print(f"❌ {description} failed:")
            print(result.stderr)
            return None
    except Exception as e:
        print(f"❌ Error during {description}: {e}")
        return None

def check_environment():
    """Check if development environment is properly set up"""
    print("🔍 Checking development environment...")
    
    # Check PlatformIO
    if run_command("pio --version", "PlatformIO version check"):
        print("   └─ PlatformIO CLI available")
    else:
        print("❌ PlatformIO not found. Please install PlatformIO first.")
        return False
    
    # Check project structure
    required_files = [
        "platformio.ini",
        "src/main.cpp",
        "partitions_littlefs_16mb.csv",
        "data/dulux.json"
    ]
    
    missing_files = []
    for file in required_files:
        if os.path.exists(file):
            print(f"   └─ ✅ {file}")
        else:
            missing_files.append(file)
            print(f"   └─ ❌ {file}")
    
    if missing_files:
        print(f"❌ Missing required files: {missing_files}")
        return False
    
    return True

def validate_data_files():
    """Validate data files before upload"""
    print("📋 Validating data files...")
    
    data_dir = "data"
    if not os.path.exists(data_dir):
        print("❌ Data directory not found")
        return False
    
    # Check dulux.json
    dulux_path = os.path.join(data_dir, "dulux.json")
    if os.path.exists(dulux_path):
        try:
            with open(dulux_path, 'r') as f:
                data = json.load(f)
                if isinstance(data, list):
                    print(f"   └─ ✅ dulux.json: {len(data)} color entries ({os.path.getsize(dulux_path):,} bytes)")
                else:
                    print("   └─ ❌ dulux.json: Not a valid array")
                    return False
        except json.JSONDecodeError as e:
            print(f"   └─ ❌ dulux.json: Invalid JSON - {e}")
            return False
    else:
        print("   └─ ❌ dulux.json not found")
        return False
    
    # Check frontend files
    frontend_files = ["index.html", "index.css", "index.js"]
    for file in frontend_files:
        file_path = os.path.join(data_dir, file)
        if os.path.exists(file_path):
            size = os.path.getsize(file_path)
            print(f"   └─ ✅ {file}: {size:,} bytes")
        else:
            print(f"   └─ ❌ {file} not found")
            return False
    
    # Calculate total size
    total_size = sum(os.path.getsize(os.path.join(data_dir, f)) 
                    for f in os.listdir(data_dir) 
                    if os.path.isfile(os.path.join(data_dir, f)))
    print(f"   └─ 📊 Total data size: {total_size:,} bytes ({total_size/1024/1024:.2f} MB)")
    
    # Check against partition size (~9.7MB)
    if total_size > 9 * 1024 * 1024:
        print("   └─ ⚠️  Warning: Data size may exceed LittleFS partition capacity")
    
    return True

def build_firmware():
    """Build the firmware"""
    output = run_command("pio run --environment um_pros3", "Building firmware")
    if output:
        # Extract build information
        lines = output.split('\n')
        for line in lines:
            if 'RAM:' in line or 'Flash:' in line:
                print(f"   └─ {line.strip()}")
        return True
    return False

def upload_filesystem():
    """Upload data files to LittleFS"""
    return run_command("pio run --target uploadfs --environment um_pros3", "Uploading filesystem")

def upload_firmware():
    """Upload firmware to device"""
    return run_command("pio run --target upload --environment um_pros3", "Uploading firmware")

def monitor_serial():
    """Start serial monitor"""
    print("🖥️  Starting serial monitor...")
    print("   Press Ctrl+C to stop monitoring")
    print("   Look for memory usage and color database loading messages")
    print("-" * 60)
    
    try:
        subprocess.run("pio device monitor --environment um_pros3", shell=True)
    except KeyboardInterrupt:
        print("\n📱 Serial monitoring stopped")

def main():
    """Main build and test workflow"""
    print("ESP32-S3 Color Sensor - Build & Test Tool")
    print("=" * 50)
    
    # Check environment
    if not check_environment():
        print("❌ Environment check failed. Please fix issues and try again.")
        sys.exit(1)
    
    # Validate data files
    if not validate_data_files():
        print("❌ Data validation failed. Please fix data files and try again.")
        sys.exit(1)
    
    print("\n🚀 Starting build process...")
    
    # Ask user what to do
    print("\nSelect build options:")
    print("1. Full build and upload (firmware + data)")
    print("2. Build firmware only")
    print("3. Upload data files only")
    print("4. Upload firmware only")
    print("5. Monitor serial output only")
    print("6. Clean and rebuild everything")
    
    choice = input("\nEnter your choice (1-6): ").strip()
    
    if choice == "1":
        # Full build and upload
        if build_firmware():
            if upload_filesystem():
                if upload_firmware():
                    print("✅ Complete build and upload successful!")
                    time.sleep(2)
                    monitor_serial()
    
    elif choice == "2":
        # Build only
        build_firmware()
    
    elif choice == "3":
        # Upload data only
        upload_filesystem()
    
    elif choice == "4":
        # Upload firmware only
        upload_firmware()
    
    elif choice == "5":
        # Monitor only
        monitor_serial()
    
    elif choice == "6":
        # Clean and rebuild
        run_command("pio run --target clean --environment um_pros3", "Cleaning build")
        if build_firmware():
            if upload_filesystem():
                if upload_firmware():
                    print("✅ Clean rebuild and upload successful!")
                    time.sleep(2)
                    monitor_serial()
    
    else:
        print("❌ Invalid choice")
        sys.exit(1)

if __name__ == "__main__":
    main()
