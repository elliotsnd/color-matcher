#!/usr/bin/env python3
"""
Live Calibration Adjustment Tool
Interactive tool for fine-tuning matrix calibration in real-time
"""
import requests
import time
import json

# Configuration
DEVICE_IP = "192.168.1.100"  # Replace with your device IP
BASE_URL = f"http://{DEVICE_IP}"

# Target values
VIVID_WHITE_TARGET = {"r": 247, "g": 248, "b": 244}
GREY_PORT_TARGET = {"r": 168, "g": 160, "b": 147}

def get_current_color():
    """Get current color reading from device"""
    try:
        response = requests.get(f"{BASE_URL}/api/color")
        if response.status_code == 200:
            return response.json()
        else:
            print(f"Error getting color: {response.status_code}")
            return None
    except Exception as e:
        print(f"Error: {e}")
        return None

def get_current_calibration():
    """Get current calibration matrix values"""
    try:
        response = requests.get(f"{BASE_URL}/api/calibration")
        if response.status_code == 200:
            return response.json()
        else:
            print(f"Error getting calibration: {response.status_code}")
            return None
    except Exception as e:
        print(f"Error: {e}")
        return None

def update_matrix(matrix_name, index, value):
    """Update a specific matrix value"""
    param_name = f"{matrix_name}{index}"
    data = {param_name: str(value)}
    
    try:
        response = requests.post(f"{BASE_URL}/api/calibration", data=data)
        if response.status_code == 200:
            print(f"✅ Updated {param_name} = {value}")
            return True
        else:
            print(f"❌ Failed to update {param_name}: {response.status_code}")
            return False
    except Exception as e:
        print(f"❌ Error updating {param_name}: {e}")
        return False

def calculate_error(current, target):
    """Calculate RGB error from target"""
    r_error = current.get('r', 0) - target['r']
    g_error = current.get('g', 0) - target['g']
    b_error = current.get('b', 0) - target['b']
    total_error = abs(r_error) + abs(g_error) + abs(b_error)
    return r_error, g_error, b_error, total_error

def display_status(current, target, target_name):
    """Display current status vs target"""
    if not current:
        print("❌ No color reading available")
        return
    
    r_err, g_err, b_err, total_err = calculate_error(current, target)
    
    print(f"\n📊 {target_name} CALIBRATION STATUS")
    print("=" * 50)
    print(f"Current RGB: ({current.get('r', 'N/A')}, {current.get('g', 'N/A')}, {current.get('b', 'N/A')})")
    print(f"Target RGB:  ({target['r']}, {target['g']}, {target['b']})")
    print(f"Error:       ({r_err:+d}, {g_err:+d}, {b_err:+d}) | Total: {total_err}")
    print(f"XYZ Values:  ({current.get('x', 'N/A')}, {current.get('y', 'N/A')}, {current.get('z', 'N/A')})")
    print(f"IR Values:   ({current.get('ir1', 'N/A')}, {current.get('ir2', 'N/A')})")
    
    # Determine which matrix is being used
    y_val = current.get('y', 0)
    if y_val > 8000:
        print(f"🌟 Using BRIGHT matrix (Y={y_val} > 8000)")
        return "bright"
    else:
        print(f"🌙 Using DARK matrix (Y={y_val} ≤ 8000)")
        return "dark"

def suggest_adjustments(current, target, matrix_type):
    """Suggest matrix adjustments based on current error"""
    if not current:
        return []
    
    r_err, g_err, b_err, total_err = calculate_error(current, target)
    suggestions = []
    
    # Simple adjustment strategy
    # If RGB is too high, reduce the corresponding matrix coefficients
    # If RGB is too low, increase the corresponding matrix coefficients
    
    adjustment_factor = 0.001  # Small adjustment step
    
    if abs(r_err) > 2:  # Red adjustment needed
        if r_err > 0:  # Too much red
            suggestions.append((f"{matrix_type}Matrix", 0, -adjustment_factor, f"Reduce red (X contribution)"))
        else:  # Too little red  
            suggestions.append((f"{matrix_type}Matrix", 0, adjustment_factor, f"Increase red (X contribution)"))
    
    if abs(g_err) > 2:  # Green adjustment needed
        if g_err > 0:  # Too much green
            suggestions.append((f"{matrix_type}Matrix", 4, -adjustment_factor, f"Reduce green (Y contribution)"))
        else:  # Too little green
            suggestions.append((f"{matrix_type}Matrix", 4, adjustment_factor, f"Increase green (Y contribution)"))
    
    if abs(b_err) > 2:  # Blue adjustment needed
        if b_err > 0:  # Too much blue
            suggestions.append((f"{matrix_type}Matrix", 8, -adjustment_factor, f"Reduce blue (Z contribution)"))
        else:  # Too little blue
            suggestions.append((f"{matrix_type}Matrix", 8, adjustment_factor, f"Increase blue (Z contribution)"))
    
    return suggestions

def interactive_calibration():
    """Interactive calibration session"""
    print("🎯 LIVE CALIBRATION ADJUSTMENT TOOL")
    print("=" * 60)
    print("This tool will help you fine-tune the matrix calibration in real-time")
    print("Place your color target on the sensor and follow the prompts")
    print()
    print("Commands:")
    print("  'v' - Switch to Vivid White target (247,248,244)")
    print("  'g' - Switch to Grey Port target (168,160,147)")
    print("  'r' - Read current color")
    print("  's' - Show suggested adjustments")
    print("  'a' - Apply suggested adjustments")
    print("  'q' - Quit")
    print()
    
    current_target = VIVID_WHITE_TARGET
    target_name = "VIVID WHITE"
    
    while True:
        print(f"\n🎯 Current Target: {target_name}")
        command = input("Enter command (v/g/r/s/a/q): ").lower().strip()
        
        if command == 'q':
            print("Exiting calibration tool...")
            break
        
        elif command == 'v':
            current_target = VIVID_WHITE_TARGET
            target_name = "VIVID WHITE"
            print("🌟 Switched to Vivid White target (247,248,244)")
            
        elif command == 'g':
            current_target = GREY_PORT_TARGET
            target_name = "GREY PORT"
            print("🌙 Switched to Grey Port target (168,160,147)")
            
        elif command == 'r':
            print("📖 Reading current color...")
            current = get_current_color()
            if current:
                matrix_type = display_status(current, current_target, target_name)
            
        elif command == 's':
            print("📖 Analyzing current reading...")
            current = get_current_color()
            if current:
                matrix_type = display_status(current, current_target, target_name)
                suggestions = suggest_adjustments(current, current_target, matrix_type)
                
                if suggestions:
                    print(f"\n💡 SUGGESTED ADJUSTMENTS:")
                    for i, (matrix, index, adjustment, description) in enumerate(suggestions):
                        print(f"  {i+1}. {description}")
                        print(f"     {matrix}{index} += {adjustment:+.4f}")
                else:
                    print("✅ No adjustments needed - within tolerance")
            
        elif command == 'a':
            print("📖 Applying suggested adjustments...")
            current = get_current_color()
            if current:
                matrix_type = display_status(current, current_target, target_name)
                suggestions = suggest_adjustments(current, current_target, matrix_type)
                
                if suggestions:
                    # Get current calibration values
                    calib = get_current_calibration()
                    if calib:
                        print(f"\n🔧 Applying adjustments...")
                        for matrix, index, adjustment, description in suggestions:
                            current_val = float(calib.get(f"{matrix}{index}", 0))
                            new_val = current_val + adjustment
                            print(f"   {description}: {current_val:.6f} → {new_val:.6f}")
                            update_matrix(matrix, index, new_val)
                        
                        # Wait a moment and read new values
                        time.sleep(1)
                        print("\n📖 Reading after adjustment...")
                        current = get_current_color()
                        if current:
                            display_status(current, current_target, target_name)
                else:
                    print("✅ No adjustments needed")
            
        else:
            print("❌ Invalid command. Use v/g/r/s/a/q")

if __name__ == "__main__":
    print("🎯 Starting Live Calibration Tool")
    print(f"🌐 Device IP: {DEVICE_IP}")
    print("⚠️  Make sure to update DEVICE_IP with your actual device IP")
    print()
    
    # Test connection
    current = get_current_color()
    if current:
        print("✅ Device connected successfully")
        interactive_calibration()
    else:
        print("❌ Cannot connect to device")
        print("   Please check device IP and network connection")
