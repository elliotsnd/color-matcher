#!/usr/bin/env python3
"""
Test script to verify the binary format is correct.
"""

import struct
import os

def test_binary_format():
    """Test the binary file format."""
    
    binary_file = "data/dulux.bin"
    
    if not os.path.exists(binary_file):
        print(f"❌ Binary file not found: {binary_file}")
        return False
    
    print(f"🔍 Testing binary file: {binary_file}")
    
    with open(binary_file, 'rb') as f:
        # Read header
        header_data = f.read(16)
        if len(header_data) != 16:
            print(f"❌ Invalid header size: {len(header_data)} bytes")
            return False
        
        magic, version, color_count, reserved = struct.unpack('<4sIII', header_data)
        
        print(f"📊 Header Information:")
        print(f"   Magic: {magic}")
        print(f"   Version: {version}")
        print(f"   Color Count: {color_count}")
        print(f"   Reserved: {reserved}")
        
        # Validate header
        if magic != b'DULX':
            print(f"❌ Invalid magic number: {magic}")
            return False
        
        if version != 1:
            print(f"❌ Invalid version: {version}")
            return False
        
        if color_count == 0:
            print(f"❌ No colors in database")
            return False
        
        print(f"✅ Header is valid")
        
        # Test reading first few color entries
        print(f"\n🎨 Testing first 3 color entries:")
        
        for i in range(min(3, color_count)):
            try:
                # Read RGB
                r = struct.unpack('B', f.read(1))[0]
                g = struct.unpack('B', f.read(1))[0]
                b = struct.unpack('B', f.read(1))[0]
                
                # Read LRV
                lrv_scaled = struct.unpack('<H', f.read(2))[0]
                lrv = lrv_scaled / 100.0
                
                # Read ID
                color_id = struct.unpack('<I', f.read(4))[0]
                
                # Read name
                name_len = struct.unpack('B', f.read(1))[0]
                name = f.read(name_len).decode('utf-8')
                
                # Read code
                code_len = struct.unpack('B', f.read(1))[0]
                code = f.read(code_len).decode('utf-8')
                
                # Read light text flag
                light_text = struct.unpack('B', f.read(1))[0] != 0
                
                print(f"   Color {i+1}:")
                print(f"     Name: {name}")
                print(f"     Code: {code}")
                print(f"     RGB: ({r}, {g}, {b})")
                print(f"     LRV: {lrv}")
                print(f"     ID: {color_id}")
                print(f"     Light Text: {light_text}")
                
            except Exception as e:
                print(f"❌ Error reading color {i+1}: {e}")
                return False
        
        print(f"\n✅ Binary format test passed!")
        
        # Check file size
        file_size = os.path.getsize(binary_file)
        print(f"\n📏 File size: {file_size:,} bytes ({file_size/1024:.1f} KB)")
        
        return True

if __name__ == "__main__":
    if test_binary_format():
        print("\n🎉 Binary file format is correct!")
    else:
        print("\n❌ Binary file format test failed!")
