#!/usr/bin/env python3
"""
Script to upload data files to ESP32-S3 LittleFS filesystem
This script uploads the frontend files and dulux.json color database to the ESP32-S3
"""

import os
import shutil
import subprocess
import sys
import json

def check_data_files():
    """Check if required data files exist and are valid"""
    data_dir = "data_clean"  # Use binary-prepared data directory
    required_files = ["index.html", "dulux.bin"]  # Binary format instead of JSON

    print("Checking binary data files...")

    if not os.path.exists(data_dir):
        print(f"❌ Error: {data_dir} directory not found")
        print("   Please run 'python prepare_binary_data.py' first")
        return False
    
    missing_files = []
    for file in required_files:
        file_path = os.path.join(data_dir, file)
        if not os.path.exists(file_path):
            missing_files.append(file)
        else:
            file_size = os.path.getsize(file_path)
            print(f"✅ {file}: {file_size:,} bytes")
            
            # Special validation for dulux.json
            if file == "dulux.json":
                try:
                    with open(file_path, 'r') as f:
                        data = json.load(f)
                        if isinstance(data, list):
                            print(f"   └─ Contains {len(data)} color entries")
                        else:
                            print(f"   └─ Warning: Expected array, got {type(data)}")
                except json.JSONDecodeError as e:
                    print(f"   └─ ❌ Invalid JSON: {e}")
                    return False
    
    if missing_files:
        print(f"❌ Missing required files: {missing_files}")
        return False
    
    # Calculate total size
    total_size = sum(os.path.getsize(os.path.join(data_dir, f)) 
                    for f in os.listdir(data_dir) 
                    if os.path.isfile(os.path.join(data_dir, f)))
    print(f"📊 Total data size: {total_size:,} bytes ({total_size/1024:.1f} KB)")
    
    # Check against partition size (~10MB available)
    if total_size > 10 * 1024 * 1024:
        print("⚠️  Warning: Data size is large, may not fit in LittleFS partition")
    
    return True

def main():
    print("ESP32-S3 Color Sensor - Data Upload Tool")
    print("=" * 50)
    
    # Check if PlatformIO is available
    try:
        result = subprocess.run(['pio', '--version'], check=True, capture_output=True, text=True)
        print(f"📦 PlatformIO version: {result.stdout.strip()}")
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("❌ Error: PlatformIO CLI not found. Please install PlatformIO first.")
        print("Visit: https://platformio.org/install/cli")
        sys.exit(1)

    # Ensure we're in the project directory
    if not os.path.exists('platformio.ini'):
        print("❌ Error: platformio.ini not found. Please run this script from the project root.")
        sys.exit(1)

    # Check partition table
    if os.path.exists('partitions_littlefs_16mb.csv'):
        print("✅ Custom partition table found: partitions_littlefs_16mb.csv")
        with open('partitions_littlefs_16mb.csv', 'r') as f:
            content = f.read()
            if 'littlefs' in content:
                # Extract littlefs partition size
                for line in content.split('\n'):
                    if 'littlefs' in line and not line.startswith('#'):
                        parts = line.split(',')
                        if len(parts) >= 5:
                            size_hex = parts[4].strip()
                            try:
                                size_bytes = int(size_hex, 16)
                                size_mb = size_bytes / (1024 * 1024)
                                print(f"   └─ LittleFS partition size: {size_mb:.1f} MB")
                            except ValueError:
                                print(f"   └─ LittleFS partition size: {size_hex}")
                        break
    else:
        print("⚠️  Using default partition table (may have limited LittleFS space)")

    # Validate data files
    if not check_data_files():
        print("❌ Data validation failed. Please fix the issues and try again.")
        sys.exit(1)

    print("\n🚀 Starting LittleFS upload...")
    
    try:
        # Upload filesystem with verbose output
        result = subprocess.run(['pio', 'run', '--target', 'uploadfs', '--environment', 'um_pros3'], 
                              capture_output=True, text=True)
        
        if result.returncode == 0:
            print("✅ Binary data files uploaded successfully to ESP32-S3!")
            print("\n📄 Upload summary:")
            print("   • Frontend files (HTML, CSS, JS) uploaded")
            print("   • Binary Dulux color database uploaded (83% smaller!)")
            print("   • Files stored in LittleFS partition")
            print("   • Memory usage significantly reduced")
            
            print("\n🔧 Next steps:")
            print("1. Verify WiFi credentials in src/main.cpp")
            print("2. Upload firmware: pio run --target upload --environment um_pros3")
            print("3. Monitor serial output: pio device monitor --environment um_pros3")
            print("4. Access web interface at: http://192.168.0.152 (or device IP)")
            
        else:
            print("❌ Upload failed:")
            print("STDOUT:", result.stdout)
            print("STDERR:", result.stderr)
            print("\n💡 Troubleshooting tips:")
            print("• Check that ESP32-S3 is connected and in download mode")
            print("• Verify COM port is correct in platformio.ini")
            print("• Try resetting the device and uploading again")
            sys.exit(1)
            
    except Exception as e:
        print(f"❌ Error during upload: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
