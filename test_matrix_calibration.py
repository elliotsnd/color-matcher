#!/usr/bin/env python3
"""
Test script to verify matrix calibration system implementation
"""
import requests

# Test configuration
DEVICE_IP = "192.168.1.100"  # Replace with your device IP
BASE_URL = f"http://{DEVICE_IP}"

def test_matrix_calibration():
    """Test the matrix calibration API endpoint with your specific parameters"""
    print("Testing Matrix Calibration System")
    print("=" * 40)
    
    # Test with your provided parameters
    print("\n1. Testing with your provided matrix calibration parameters...")
    
    # Your specific parameters
    matrix_params = {
        "brightMatrix0": "0.1054",
        "brightMatrix1": "-0.017",
        "brightMatrix2": "-0.026",
        "brightMatrix3": "-0.017",
        "brightMatrix4": "0.0785",
        "brightMatrix5": "0.0017",
        "brightMatrix6": "0.0052",
        "brightMatrix7": "-0.01",
        "brightMatrix8": "0.1268",
        "useMatrixCalibration": "true",
        "enableDynamicCalibration": "true",
        "dynamicThreshold": "8000"
    }
    
    response = requests.post(f"{BASE_URL}/api/calibration", data=matrix_params)
    if response.status_code == 200:
        result = response.json()
        print(f"✓ Matrix calibration updated successfully: {result}")
    else:
        print(f"✗ Failed to update matrix calibration: {response.status_code}")
        print(f"Response: {response.text}")
        return False
    
    # Test 2: Get current settings to verify
    print("\n2. Getting current settings to verify matrix calibration...")
    response = requests.get(f"{BASE_URL}/api/settings")
    if response.status_code == 200:
        result = response.json()
        print(f"✓ Current settings retrieved:")
        print(f"  useMatrixCalibration: {result.get('useMatrixCalibration', 'Not found')}")
        print(f"  enableDynamicCalibration: {result.get('enableDynamicCalibration', 'Not found')}")
        print(f"  dynamicThreshold: {result.get('dynamicThreshold', 'Not found')}")
    else:
        print(f"✗ Failed to get settings: {response.status_code}")
        return False
    
    # Test 3: Get current calibration values
    print("\n3. Getting current calibration values...")
    response = requests.get(f"{BASE_URL}/api/calibration")
    if response.status_code == 200:
        result = response.json()
        print(f"✓ Current calibration values retrieved:")
        # Print matrix values
        for i in range(9):
            key = f"brightMatrix{i}"
            if key in result:
                print(f"  {key}: {result[key]}")
    else:
        print(f"✗ Failed to get calibration values: {response.status_code}")
        return False
    
    # Test 4: Get color reading with matrix calibration
    print("\n4. Testing color reading with matrix calibration...")
    response = requests.get(f"{BASE_URL}/api/color")
    if response.status_code == 200:
        result = response.json()
        print(f"✓ Color reading with matrix calibration:")
        print(f"  RGB: ({result.get('r', 'N/A')}, {result.get('g', 'N/A')}, {result.get('b', 'N/A')})")
        print(f"  XYZ: ({result.get('x', 'N/A')}, {result.get('y', 'N/A')}, {result.get('z', 'N/A')})")
        print(f"  IR: ({result.get('ir1', 'N/A')}, {result.get('ir2', 'N/A')})")
    else:
        print(f"✗ Failed to get color reading: {response.status_code}")
        return False
    
    print("\n✓ All matrix calibration tests passed!")
    return True

def test_api_endpoints():
    """Test all the API endpoints mentioned in the request"""
    print("\nTesting API Endpoints:")
    print("=" * 30)
    
    # Test GET /api/calibration
    print("\n1. Testing GET /api/calibration...")
    response = requests.get(f"{BASE_URL}/api/calibration")
    if response.status_code == 200:
        print("✓ GET /api/calibration - SUCCESS")
    else:
        print(f"✗ GET /api/calibration - FAILED ({response.status_code})")
    
    # Test POST /api/calibration
    print("\n2. Testing POST /api/calibration...")
    test_data = {"useMatrixCalibration": "true"}
    response = requests.post(f"{BASE_URL}/api/calibration", data=test_data)
    if response.status_code == 200:
        print("✓ POST /api/calibration - SUCCESS")
    else:
        print(f"✗ POST /api/calibration - FAILED ({response.status_code})")
    
    # Test POST /api/tune-vivid-white
    print("\n3. Testing POST /api/tune-vivid-white...")
    response = requests.post(f"{BASE_URL}/api/tune-vivid-white", data={})
    if response.status_code == 200:
        print("✓ POST /api/tune-vivid-white - SUCCESS")
    else:
        print(f"✗ POST /api/tune-vivid-white - FAILED ({response.status_code})")
    
    # Test GET /api/settings
    print("\n4. Testing GET /api/settings...")
    response = requests.get(f"{BASE_URL}/api/settings")
    if response.status_code == 200:
        print("✓ GET /api/settings - SUCCESS")
    else:
        print(f"✗ GET /api/settings - FAILED ({response.status_code})")

if __name__ == "__main__":
    print("Matrix Calibration System Test")
    print("Replace DEVICE_IP with your actual device IP address")
    print("Make sure your device is connected to the same network")
    print()
    
    try:
        test_matrix_calibration()
        test_api_endpoints()
    except Exception as e:
        print(f"Test failed with error: {e}")
        print("Make sure your device is running and accessible")
