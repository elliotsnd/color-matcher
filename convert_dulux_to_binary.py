#!/usr/bin/env python3
"""
Convert dulux.json to a compact binary format for ESP32 memory efficiency.

Binary Format Structure:
- Header (16 bytes):
  - Magic number: "DULX" (4 bytes)
  - Version: uint32 (4 bytes) 
  - Color count: uint32 (4 bytes)
  - Reserved: uint32 (4 bytes)

- Color Entry (variable length):
  - RGB values: 3 bytes (r, g, b)
  - LRV: uint16 (scaled by 100, so 79.40 becomes 7940)
  - ID: uint32
  - Name length: uint8 (max 255 chars)
  - Name: variable length string (UTF-8)
  - Code length: uint8 (max 255 chars) 
  - Code: variable length string (UTF-8)
  - Light text flag: uint8 (0 or 1)

This format reduces memory usage by:
1. Using fixed-size integers instead of strings for numeric values
2. Storing strings with length prefixes instead of null-terminated
3. Packing RGB values into 3 bytes instead of separate string fields
4. Using uint16 for LRV (scaled by 100) instead of string
"""

import json
import struct
import os
import sys

def convert_dulux_to_binary(json_file, binary_file):
    """Convert dulux.json to binary format."""
    
    print(f"🔄 Converting {json_file} to binary format...")
    
    # Load JSON data
    try:
        with open(json_file, 'r', encoding='utf-8') as f:
            colors = json.load(f)
    except Exception as e:
        print(f"❌ Error loading JSON: {e}")
        return False
    
    if not isinstance(colors, list):
        print("❌ Error: JSON should contain an array of colors")
        return False
    
    print(f"📊 Found {len(colors)} colors to convert")
    
    # Open binary file for writing
    try:
        with open(binary_file, 'wb') as f:
            # Write header
            magic = b'DULX'
            version = 1
            color_count = len(colors)
            reserved = 0
            
            f.write(magic)
            f.write(struct.pack('<I', version))      # Little-endian uint32
            f.write(struct.pack('<I', color_count))  # Little-endian uint32
            f.write(struct.pack('<I', reserved))     # Little-endian uint32
            
            print(f"📝 Written header: magic={magic}, version={version}, count={color_count}")
            
            # Write color entries
            for i, color in enumerate(colors):
                try:
                    # Extract and validate data
                    name = str(color.get('name', '')).strip()
                    code = str(color.get('code', '')).strip()
                    r = int(color.get('r', 0))
                    g = int(color.get('g', 0))
                    b = int(color.get('b', 0))
                    lrv_str = str(color.get('lrv', '0'))
                    id_val = int(color.get('id', 0))
                    light_text = bool(color.get('lightText', False))
                    
                    # Convert LRV string to scaled integer (e.g., "79.40" -> 7940)
                    try:
                        lrv_float = float(lrv_str)
                        lrv_scaled = int(lrv_float * 100)
                    except:
                        lrv_scaled = 0
                    
                    # Validate ranges
                    if not (0 <= r <= 255 and 0 <= g <= 255 and 0 <= b <= 255):
                        print(f"⚠️  Warning: Invalid RGB values for color {i}: ({r}, {g}, {b})")
                        continue
                    
                    if lrv_scaled > 65535:  # uint16 max
                        print(f"⚠️  Warning: LRV too large for color {i}: {lrv_scaled}")
                        lrv_scaled = 65535
                    
                    if len(name.encode('utf-8')) > 255:
                        print(f"⚠️  Warning: Name too long for color {i}, truncating")
                        name = name[:200]  # Leave some room for UTF-8 encoding
                    
                    if len(code.encode('utf-8')) > 255:
                        print(f"⚠️  Warning: Code too long for color {i}, truncating")
                        code = code[:200]
                    
                    # Encode strings to UTF-8
                    name_bytes = name.encode('utf-8')
                    code_bytes = code.encode('utf-8')
                    
                    # Write color entry
                    f.write(struct.pack('BBB', r, g, b))           # RGB (3 bytes)
                    f.write(struct.pack('<H', lrv_scaled))         # LRV scaled (2 bytes)
                    f.write(struct.pack('<I', id_val))             # ID (4 bytes)
                    f.write(struct.pack('B', len(name_bytes)))     # Name length (1 byte)
                    f.write(name_bytes)                            # Name (variable)
                    f.write(struct.pack('B', len(code_bytes)))     # Code length (1 byte)
                    f.write(code_bytes)                            # Code (variable)
                    f.write(struct.pack('B', 1 if light_text else 0))  # Light text flag (1 byte)
                    
                    if (i + 1) % 500 == 0:
                        print(f"📝 Processed {i + 1}/{len(colors)} colors...")
                        
                except Exception as e:
                    print(f"❌ Error processing color {i}: {e}")
                    print(f"   Color data: {color}")
                    continue
            
            print(f"✅ Binary conversion complete!")
            
    except Exception as e:
        print(f"❌ Error writing binary file: {e}")
        return False
    
    # Show file size comparison
    try:
        json_size = os.path.getsize(json_file)
        binary_size = os.path.getsize(binary_file)
        compression_ratio = (1 - binary_size / json_size) * 100
        
        print(f"\n📊 File Size Comparison:")
        print(f"   JSON:   {json_size:,} bytes ({json_size/1024:.1f} KB)")
        print(f"   Binary: {binary_size:,} bytes ({binary_size/1024:.1f} KB)")
        print(f"   Saved:  {json_size - binary_size:,} bytes ({compression_ratio:.1f}% reduction)")
        
    except Exception as e:
        print(f"⚠️  Could not calculate file sizes: {e}")
    
    return True

def main():
    """Main function."""
    
    # Default file paths
    json_file = "data/dulux.json"
    binary_file = "data/dulux.bin"
    
    # Check if JSON file exists
    if not os.path.exists(json_file):
        print(f"❌ Error: {json_file} not found")
        print("   Available dulux.json files:")
        for root, dirs, files in os.walk("."):
            for file in files:
                if file == "dulux.json":
                    print(f"   - {os.path.join(root, file)}")
        return 1
    
    # Convert to binary
    if convert_dulux_to_binary(json_file, binary_file):
        print(f"\n🎉 Success! Binary file created: {binary_file}")
        print(f"📁 You can now upload this binary file to the ESP32 instead of the JSON file")
        return 0
    else:
        print(f"\n❌ Conversion failed")
        return 1

if __name__ == "__main__":
    sys.exit(main())
