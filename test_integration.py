#!/usr/bin/env python3
"""
Test script to verify the color sensor web interface integration
This script checks if all necessary files are in place and provides setup guidance
"""

import os
import json

def check_file_exists(filepath, description):
    """Check if a file exists and print status"""
    if os.path.exists(filepath):
        print(f"✅ {description}: {filepath}")
        return True
    else:
        print(f"❌ {description}: {filepath} - NOT FOUND")
        return False

def check_arduino_code():
    """Check Arduino code for required components"""
    print("\n=== Arduino Code Check ===")
    
    if not check_file_exists("src/main.cpp", "Arduino main file"):
        return False
    
    with open("src/main.cpp", "r") as f:
        content = f.read()
    
    required_includes = [
        "#include <ESP8266WiFi.h>",
        "#include <ESP8266WebServer.h>",
        "#include <ArduinoJson.h>",
        "#include <LittleFS.h>"
    ]
    
    for include in required_includes:
        if include in content:
            print(f"✅ Found: {include}")
        else:
            print(f"❌ Missing: {include}")
    
    if "handleColorAPI" in content:
        print("✅ Found: Color API handler")
    else:
        print("❌ Missing: Color API handler")
    
    if "server.begin()" in content:
        print("✅ Found: Web server initialization")
    else:
        print("❌ Missing: Web server initialization")

def check_frontend_files():
    """Check frontend files"""
    print("\n=== Frontend Files Check ===")
    
    files_to_check = [
        ("data/index.html", "Main HTML file"),
        ("data/index.css", "Stylesheet"),
        ("data/index.tsx", "React application"),
        ("data/package.json", "Package configuration")
    ]
    
    all_present = True
    for filepath, description in files_to_check:
        if not check_file_exists(filepath, description):
            all_present = False
    
    return all_present

def check_platformio_config():
    """Check PlatformIO configuration"""
    print("\n=== PlatformIO Configuration Check ===")
    
    if not check_file_exists("platformio.ini", "PlatformIO config"):
        return False
    
    with open("platformio.ini", "r") as f:
        content = f.read()
    
    if "board = nodemcu" in content:
        print("✅ Board: NodeMCU ESP8266")
    else:
        print("❌ Board configuration issue")
    
    if "bblanchon/ArduinoJson" in content:
        print("✅ ArduinoJson library configured")
    else:
        print("❌ ArduinoJson library missing")
    
    if "board_build.filesystem = littlefs" in content:
        print("✅ LittleFS filesystem configured")
    else:
        print("❌ LittleFS filesystem not configured")

def check_wifi_config():
    """Check WiFi configuration in Arduino code"""
    print("\n=== WiFi Configuration Check ===")
    
    with open("src/main.cpp", "r") as f:
        content = f.read()
    
    if 'ssid = "YOUR_WIFI_SSID"' in content:
        print("⚠️  WiFi SSID not configured - update src/main.cpp")
    else:
        print("✅ WiFi SSID appears to be configured")
    
    if 'password = "YOUR_WIFI_PASSWORD"' in content:
        print("⚠️  WiFi password not configured - update src/main.cpp")
    else:
        print("✅ WiFi password appears to be configured")

def main():
    print("=== Color Sensor Web Interface Integration Test ===")
    
    # Check if we're in the right directory
    if not os.path.exists("platformio.ini"):
        print("❌ Error: platformio.ini not found. Please run this script from the project root.")
        return
    
    check_arduino_code()
    check_frontend_files()
    check_platformio_config()
    check_wifi_config()
    
    print("\n=== Next Steps ===")
    print("1. Update WiFi credentials in src/main.cpp")
    print("2. Upload frontend files: python upload_data.py")
    print("3. Compile and upload: pio run --target upload")
    print("4. Monitor serial: pio device monitor")
    print("5. Open web browser to the IP address shown in serial monitor")
    
    print("\n=== API Endpoints ===")
    print("GET /              - Main web interface")
    print("GET /api/color     - JSON color data")
    print("GET /index.css     - Stylesheet")
    print("GET /index.tsx     - JavaScript application")

if __name__ == "__main__":
    main()
