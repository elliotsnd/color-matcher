#!/usr/bin/env python3
"""
ESP32 Partition and LittleFS Space Monitor
Helps check partition sizes and monitor filesystem usage
"""

import subprocess
import sys
import os

def run_command(cmd, check=True):
    """Run a command and return the result"""
    try:
        result = subprocess.run(cmd, shell=True, check=check, 
                              capture_output=True, text=True)
        return result
    except subprocess.CalledProcessError as e:
        print(f"❌ Command failed: {cmd}")
        print(f"Error: {e.stderr}")
        return None

def check_partition_table():
    """Check the current partition table"""
    print("🔍 Checking ESP32 Partition Table...")
    
    # Look for partition files
    partition_files = [
        "partitions.csv",
        "boards/partitions.csv", 
        ".pio/build/um_pros3/partitions.bin"
    ]
    
    for pfile in partition_files:
        if os.path.exists(pfile):
            print(f"📁 Found partition file: {pfile}")
            if pfile.endswith('.csv'):
                try:
                    with open(pfile, 'r') as f:
                        print("📋 Partition Table:")
                        for line in f:
                            if line.strip() and not line.startswith('#'):
                                print(f"   {line.strip()}")
                except Exception as e:
                    print(f"❌ Error reading {pfile}: {e}")
        else:
            print(f"❌ Not found: {pfile}")

def monitor_serial():
    """Monitor serial output for LittleFS info"""
    print("\n📡 Starting serial monitor to check LittleFS usage...")
    print("Press Ctrl+C to stop monitoring")
    print("Look for 'LittleFS Total/Used/Free' messages after ESP32 reset")
    
    try:
        result = run_command("pio device monitor --environment um_pros3 --filter esp32_exception_decoder", check=False)
    except KeyboardInterrupt:
        print("\n✅ Monitoring stopped")

def check_build_size():
    """Check the size of built filesystem"""
    print("\n📊 Checking Build Sizes...")
    
    # Check LittleFS image size
    littlefs_bin = ".pio/build/um_pros3/littlefs.bin"
    if os.path.exists(littlefs_bin):
        size = os.path.getsize(littlefs_bin)
        print(f"📦 LittleFS image: {size:,} bytes ({size/1024:.1f} KB)")
        
        # Estimate compression
        data_dir = "data"
        if os.path.exists(data_dir):
            total_source = 0
            for root, dirs, files in os.walk(data_dir):
                for file in files:
                    if not file.startswith('.'):
                        filepath = os.path.join(root, file)
                        total_source += os.path.getsize(filepath)
            
            print(f"📁 Source files: {total_source:,} bytes ({total_source/1024:.1f} KB)")
            if total_source > 0:
                compression = (1 - size/total_source) * 100
                print(f"🗜️  Compression: {compression:.1f}%")
    else:
        print("❌ LittleFS image not found. Run 'pio run -t buildfs' first")
    
    # Check firmware size
    firmware_bin = ".pio/build/um_pros3/firmware.bin"
    if os.path.exists(firmware_bin):
        size = os.path.getsize(firmware_bin)
        print(f"💾 Firmware: {size:,} bytes ({size/1024:.1f} KB)")

def suggest_partition_fix():
    """Suggest partition table improvements"""
    print("\n💡 Partition Optimization Suggestions:")
    print("If you're getting 'filesystem full' errors:")
    print()
    print("1. **Current setup** (if using default):")
    print("   - app0: ~1.3MB")
    print("   - app1: ~1.3MB (OTA)")  
    print("   - spiffs: ~1.5MB")
    print()
    print("2. **Recommended for this project**:")
    print("   - app0: ~2MB (main firmware)")
    print("   - littlefs: ~4-8MB (web files + color database)")
    print("   - Remove OTA partition if not needed")
    print()
    print("3. **Create custom partitions.csv**:")
    print("```")
    print("# Name,   Type, SubType, Offset,  Size,     Flags")
    print("nvs,      data, nvs,     0x9000,  0x5000,")
    print("otadata,  data, ota,     0xe000,  0x2000,")
    print("app0,     app,  ota_0,   0x10000, 0x200000,")
    print("littlefs, data, spiffs,  0x210000,0x7F0000,")
    print("```")
    print()
    print("4. **Add to platformio.ini**:")
    print("```")
    print("[env:um_pros3]")
    print("board_build.partitions = partitions.csv")
    print("```")

def main():
    print("ESP32 Partition and LittleFS Monitor")
    print("=" * 40)
    
    if len(sys.argv) > 1:
        command = sys.argv[1]
        if command == "monitor":
            monitor_serial()
        elif command == "size":
            check_build_size()
        elif command == "partitions":
            check_partition_table()
        elif command == "suggest":
            suggest_partition_fix()
        else:
            print(f"Unknown command: {command}")
    else:
        # Run all checks
        check_partition_table()
        check_build_size()
        suggest_partition_fix()
        
        print("\n🔧 Available commands:")
        print("python check_partitions.py monitor     - Monitor serial for LittleFS info")
        print("python check_partitions.py size       - Check build sizes")
        print("python check_partitions.py partitions - Show partition table")
        print("python check_partitions.py suggest    - Show optimization suggestions")

if __name__ == "__main__":
    main()
