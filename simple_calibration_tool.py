#!/usr/bin/env python3
"""
Simple Matrix Calibration Tool
Manual adjustment tool for fine-tuning matrix values
"""
import requests

# Configuration - UPDATE THIS WITH YOUR DEVICE IP
DEVICE_IP = "192.168.0.152"  # Replace with your actual device IP
BASE_URL = f"http://{DEVICE_IP}"

def get_color():
    """Get current color reading"""
    try:
        response = requests.get(f"{BASE_URL}/api/color")
        if response.status_code == 200:
            data = response.json()
            return data
        else:
            print(f"Error: {response.status_code}")
            return None
    except Exception as e:
        print(f"Connection error: {e}")
        return None

def update_matrix_value(matrix_name, index, value):
    """Update a specific matrix value"""
    param_name = f"{matrix_name}{index}"
    data = {param_name: str(value)}
    
    try:
        response = requests.post(f"{BASE_URL}/api/calibration", data=data)
        if response.status_code == 200:
            print(f"✅ Updated {param_name} = {value}")
            return True
        else:
            print(f"❌ Failed: {response.status_code}")
            return False
    except Exception as e:
        print(f"❌ Error: {e}")
        return False

def show_current_reading():
    """Show current color reading"""
    data = get_color()
    if data:
        print(f"\n📊 Current Reading:")
        print(f"   RGB: ({data.get('r', 'N/A')}, {data.get('g', 'N/A')}, {data.get('b', 'N/A')})")
        print(f"   XYZ: ({data.get('x', 'N/A')}, {data.get('y', 'N/A')}, {data.get('z', 'N/A')})")
        print(f"   IR:  ({data.get('ir1', 'N/A')}, {data.get('ir2', 'N/A')})")
        
        # Show which matrix is being used
        y_val = data.get('y', 0)
        if y_val > 8000:
            print(f"   🌟 Using BRIGHT matrix (Y={y_val} > 8000)")
        else:
            print(f"   🌙 Using DARK matrix (Y={y_val} ≤ 8000)")
        
        return data
    return None

def main():
    """Main calibration function"""
    print("🎯 SIMPLE MATRIX CALIBRATION TOOL")
    print("=" * 50)
    print("TARGETS:")
    print("  Vivid White: RGB(247, 248, 244)")
    print("  Grey Port:   RGB(168, 160, 147)")
    print()
    print("MATRIX INDICES:")
    print("  Bright Matrix: brightMatrix0-8")
    print("  Dark Matrix:   darkMatrix0-8")
    print()
    print("COMMON ADJUSTMENTS:")
    print("  RGB too high? Reduce matrix values")
    print("  RGB too low?  Increase matrix values")
    print()
    
    # Test connection
    print("Testing connection...")
    data = show_current_reading()
    if not data:
        print("❌ Cannot connect to device")
        print(f"   Check that device IP is correct: {DEVICE_IP}")
        return
    
    print("✅ Connected successfully!")
    print()
    
    while True:
        print("\nCommands:")
        print("  r - Read current color")
        print("  u - Update matrix value")
        print("  q - Quit")
        
        cmd = input("\nEnter command: ").lower().strip()
        
        if cmd == 'q':
            print("Goodbye!")
            break
        
        elif cmd == 'r':
            show_current_reading()
        
        elif cmd == 'u':
            print("\nUpdate Matrix Value:")
            print("Matrix types: brightMatrix, darkMatrix")
            print("Indices: 0-8")
            print("Example: brightMatrix 0 0.1054")
            
            try:
                matrix_input = input("Enter matrix name: ").strip()
                index_input = input("Enter index (0-8): ").strip()
                value_input = input("Enter new value: ").strip()
                
                matrix_name = matrix_input
                index = int(index_input)
                value = float(value_input)
                
                if matrix_name not in ['brightMatrix', 'darkMatrix']:
                    print("❌ Invalid matrix name")
                    continue
                
                if index < 0 or index > 8:
                    print("❌ Invalid index (must be 0-8)")
                    continue
                
                print(f"Updating {matrix_name}{index} = {value}")
                if update_matrix_value(matrix_name, index, value):
                    print("✅ Update successful!")
                    print("Reading new values...")
                    show_current_reading()
                
            except ValueError:
                print("❌ Invalid input format")
            except Exception as e:
                print(f"❌ Error: {e}")
        
        else:
            print("❌ Invalid command")

if __name__ == "__main__":
    main()
