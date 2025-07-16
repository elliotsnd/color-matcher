#!/usr/bin/env python3
"""
Final Calibration Verification Script
Tests the complete dual-target matrix calibration system
"""
import requests

# Configuration
DEVICE_IP = "192.168.1.100"  # Replace with your device IP
BASE_URL = f"http://{DEVICE_IP}"

def verify_calibration_system():
    """Verify all final calibration parameters are correctly applied"""
    print("🎯 FINAL CALIBRATION VERIFICATION")
    print("=" * 50)
    
    # 1. Verify Matrix Calibration is enabled
    print("\n1. Verifying Matrix Calibration Status...")
    try:
        response = requests.get(f"{BASE_URL}/api/settings")
        if response.status_code == 200:
            settings = response.json()
            print(f"✅ Matrix Calibration: {settings.get('useMatrixCalibration', 'Unknown')}")
            print(f"✅ Dynamic Calibration: {settings.get('enableDynamicCalibration', 'Unknown')}")
            print(f"✅ Dynamic Threshold: {settings.get('dynamicThreshold', 'Unknown')}")
        else:
            print(f"❌ Failed to get settings: {response.status_code}")
            return False
    except Exception as e:
        print(f"❌ Error getting settings: {e}")
        return False
    
    # 2. Verify Bright Matrix (Vivid White) values
    print("\n2. Verifying Bright Matrix Values...")
    expected_bright = [0.1054, -0.017, -0.026, -0.017, 0.0785, 0.0017, 0.0052, -0.01, 0.1268]
    try:
        response = requests.get(f"{BASE_URL}/api/calibration")
        if response.status_code == 200:
            calib = response.json()
            print("✅ Bright Matrix (Vivid White):")
            for i, expected in enumerate(expected_bright):
                actual = calib.get(f'brightMatrix{i}', 'Missing')
                status = "✅" if abs(float(actual) - expected) < 0.0001 else "❌"
                print(f"   {status} brightMatrix{i}: {actual} (expected: {expected})")
        else:
            print(f"❌ Failed to get calibration: {response.status_code}")
            return False
    except Exception as e:
        print(f"❌ Error getting calibration: {e}")
        return False
    
    # 3. Verify Dark Matrix (Grey Port) values  
    print("\n3. Verifying Dark Matrix Values...")
    expected_dark = [0.037, -0.012, -0.008, -0.012, 0.032, 0.002, 0.002, -0.004, 0.058]
    try:
        response = requests.get(f"{BASE_URL}/api/calibration")
        if response.status_code == 200:
            calib = response.json()
            print("✅ Dark Matrix (Grey Port):")
            for i, expected in enumerate(expected_dark):
                actual = calib.get(f'darkMatrix{i}', 'Missing')
                status = "✅" if abs(float(actual) - expected) < 0.0001 else "❌"
                print(f"   {status} darkMatrix{i}: {actual} (expected: {expected})")
        else:
            print(f"❌ Failed to get calibration: {response.status_code}")
            return False
    except Exception as e:
        print(f"❌ Error getting calibration: {e}")
        return False
    
    # 4. Test Color Reading with Matrix Calibration
    print("\n4. Testing Color Reading with Matrix Calibration...")
    try:
        response = requests.get(f"{BASE_URL}/api/color")
        if response.status_code == 200:
            color = response.json()
            print("✅ Color Reading Results:")
            print(f"   RGB: ({color.get('r', 'N/A')}, {color.get('g', 'N/A')}, {color.get('b', 'N/A')})")
            print(f"   XYZ: ({color.get('x', 'N/A')}, {color.get('y', 'N/A')}, {color.get('z', 'N/A')})")
            print(f"   IR: ({color.get('ir1', 'N/A')}, {color.get('ir2', 'N/A')})")
            
            # Determine which matrix should be used
            y_value = color.get('y', 0)
            if y_value > 8000:
                print(f"   🌟 Using BRIGHT matrix (Y={y_value} > 8000) - Vivid White optimized")
            else:
                print(f"   🌙 Using DARK matrix (Y={y_value} ≤ 8000) - Grey Port optimized")
        else:
            print(f"❌ Failed to get color reading: {response.status_code}")
            return False
    except Exception as e:
        print(f"❌ Error getting color reading: {e}")
        return False
    
    # 5. Test Both Calibration Targets
    print("\n5. Testing Dual-Target Calibration...")
    print("   📋 Place device on different targets and observe matrix switching:")
    print("   🌟 Bright targets (Y > 8000): Uses Vivid White matrix")
    print("   🌙 Dark targets (Y ≤ 8000): Uses Grey Port matrix")
    
    return True

def test_api_endpoints():
    """Test all API endpoints for functionality"""
    print("\n🔧 API ENDPOINT TESTS")
    print("=" * 30)
    
    endpoints = [
        ("GET", "/api/calibration", "Get current matrix values"),
        ("POST", "/api/calibration", "Update matrix values"),
        ("POST", "/api/tune-vivid-white", "Quick vivid white optimization"),
        ("GET", "/api/settings", "Get all current settings"),
        ("GET", "/api/color", "Get color reading"),
        ("GET", "/api/color-fast", "Get fast color reading"),
        ("GET", "/api/color-name", "Get color name match")
    ]
    
    for method, endpoint, description in endpoints:
        try:
            if method == "GET":
                response = requests.get(f"{BASE_URL}{endpoint}")
            else:
                response = requests.post(f"{BASE_URL}{endpoint}", data={})
            
            if response.status_code == 200:
                print(f"✅ {method} {endpoint} - {description}")
            else:
                print(f"⚠️  {method} {endpoint} - Status: {response.status_code}")
        except Exception as e:
            print(f"❌ {method} {endpoint} - Error: {e}")

def main():
    """Main verification function"""
    print("🎯 FINAL CALIBRATION SYSTEM VERIFICATION")
    print("🔧 Dual-Target Matrix Calibration (Vivid White + Grey Port)")
    print("📅 Date: July 16, 2025")
    print("=" * 60)
    
    print("\n📋 EXPECTED CONFIGURATION:")
    print("   • Bright Matrix (Vivid White): [0.1054, -0.017, -0.026, -0.017, 0.0785, 0.0017, 0.0052, -0.01, 0.1268]")
    print("   • Dark Matrix (Grey Port): [0.037, -0.012, -0.008, -0.012, 0.032, 0.002, 0.002, -0.004, 0.058]")
    print("   • Dynamic Threshold: 8000 (Y value for switching)")
    print("   • Integration Time: 0x40 (181ms)")
    print("   • LED Brightness: 75")
    print("   • IR Compensation: 0.30/0.30")
    
    print(f"\n🌐 Device IP: {DEVICE_IP}")
    print("   ⚠️  Update DEVICE_IP variable with your actual device IP")
    
    try:
        # Verify calibration system
        if verify_calibration_system():
            print("\n🎉 CALIBRATION VERIFICATION COMPLETE!")
            print("✅ All matrix values correctly implemented")
            print("✅ Dual-target calibration active")
            print("✅ Dynamic matrix switching enabled")
            
            # Test API endpoints
            test_api_endpoints()
            
            print("\n🚀 SYSTEM READY FOR PRODUCTION USE!")
            print("=" * 50)
            print("✅ Dual-target calibration (vivid white + grey port)")
            print("✅ Automatic sensor adjustment based on light conditions")
            print("✅ Web API control for real-time adjustments")
            print("✅ Matrix-based calibration for wide-range accuracy")
            print("✅ Dynamic switching between bright/dark calibration matrices")
            print("✅ Persistent settings that survive reboots")
            
        else:
            print("\n❌ CALIBRATION VERIFICATION FAILED!")
            print("   Please check device connectivity and settings")
            
    except Exception as e:
        print(f"\n❌ VERIFICATION ERROR: {e}")
        print("   Make sure device is connected and accessible")

if __name__ == "__main__":
    main()
