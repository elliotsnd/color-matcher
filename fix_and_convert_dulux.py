#!/usr/bin/env python3
"""
Fix and convert Dulux JSON color database to binary format for ESP32
"""

import json
import struct
import os
import re

def fix_json_file(input_file, output_file):
    """Fix JSON formatting issues and create proper JSON array"""
    print(f"Fixing JSON file: {input_file}")
    
    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Remove the opening bracket if it exists
    content = content.lstrip()
    if content.startswith('['):
        content = content[1:]
    
    # Remove the closing bracket if it exists
    content = content.rstrip()
    if content.endswith(']'):
        content = content[:-1]
    
    # Split into individual objects
    objects = []
    current_obj = ""
    brace_count = 0
    in_string = False
    escape_next = False
    
    for char in content:
        if escape_next:
            current_obj += char
            escape_next = False
            continue
            
        if char == '\\':
            escape_next = True
            current_obj += char
            continue
            
        if char == '"' and not escape_next:
            in_string = not in_string
            
        if not in_string:
            if char == '{':
                brace_count += 1
            elif char == '}':
                brace_count -= 1
                
        current_obj += char
        
        # When we complete an object
        if brace_count == 0 and current_obj.strip().endswith('}'):
            obj_str = current_obj.strip()
            if obj_str:
                objects.append(obj_str)
            current_obj = ""
    
    print(f"Found {len(objects)} color objects")
    
    # Parse and validate each object
    valid_colors = []
    for i, obj_str in enumerate(objects):
        try:
            # Try to parse the object
            color = json.loads(obj_str)
            
            # Validate required fields
            if all(field in color for field in ['name', 'r', 'g', 'b']):
                # Ensure we have default values for missing fields
                if 'code' not in color or color['code'] == 'N/A':
                    color['code'] = f"C{i+1:05d}"
                if 'lrv' not in color or color['lrv'] == 'N/A':
                    color['lrv'] = "50.0"
                if 'id' not in color:
                    color['id'] = str(i + 1)
                if 'lightText' not in color:
                    # Determine if light text is needed based on RGB
                    r, g, b = int(color['r']), int(color['g']), int(color['b'])
                    luminance = (0.299 * r + 0.587 * g + 0.114 * b)
                    color['lightText'] = luminance < 128
                
                valid_colors.append(color)
            else:
                print(f"Skipping invalid color object {i+1}: missing required fields")
                
        except json.JSONDecodeError as e:
            print(f"Skipping invalid JSON object {i+1}: {e}")
            continue
    
    print(f"Valid colors: {len(valid_colors)}")
    
    # Write fixed JSON
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(valid_colors, f, indent=2)
    
    print(f"Fixed JSON saved to: {output_file}")
    return len(valid_colors)

def create_binary_database(json_file, output_file):
    """Convert JSON color database to binary format"""
    
    # Load the JSON database
    with open(json_file, 'r', encoding='utf-8') as f:
        colors = json.load(f)
    
    print(f"Converting {len(colors)} colors to binary format...")
    
    # Create binary file with simple format for dulux_simple_reader.h
    with open(output_file, 'wb') as f:
        # Write 16-byte header
        header = bytearray(16)
        
        # Magic number (4 bytes): "DULX"
        header[0:4] = b'DULX'
        
        # Version (4 bytes): 1
        struct.pack_into('<I', header, 4, 1)
        
        # Color count (4 bytes)
        struct.pack_into('<I', header, 8, len(colors))
        
        # Reserved (4 bytes)
        struct.pack_into('<I', header, 12, 0)
        
        f.write(header)
        
        # Write color entries
        for i, color in enumerate(colors):
            # Extract and validate data
            name = str(color['name'])[:31]  # Max 31 chars + null terminator
            code = str(color['code'])[:15]  # Max 15 chars + null terminator
            r = max(0, min(255, int(color['r'])))
            g = max(0, min(255, int(color['g'])))
            b = max(0, min(255, int(color['b'])))
            
            try:
                lrv = float(color['lrv'])
            except (ValueError, TypeError):
                lrv = 50.0  # Default LRV
            
            # Write name (null-terminated string)
            name_bytes = name.encode('utf-8')
            f.write(name_bytes)
            f.write(b'\x00')  # Null terminator
            
            # Write code (null-terminated string)
            code_bytes = code.encode('utf-8')
            f.write(code_bytes)
            f.write(b'\x00')  # Null terminator
            
            # Write RGB values (3 bytes)
            f.write(struct.pack('BBB', r, g, b))
            
            # Write LRV as float (4 bytes)
            f.write(struct.pack('<f', lrv))
            
            if (i + 1) % 1000 == 0:
                print(f"Processed {i+1}/{len(colors)} colors...")
    
    file_size = os.path.getsize(output_file)
    print(f"Created binary database: {output_file}")
    print(f"File size: {file_size:,} bytes ({file_size/1024:.1f} KB)")
    print(f"Colors: {len(colors)}")

def main():
    input_file = "data/dulux.json"
    fixed_file = "data/dulux_fixed.json"
    binary_file = "data/dulux.bin"
    
    if not os.path.exists(input_file):
        print(f"Error: {input_file} not found!")
        return
    
    # Step 1: Fix JSON formatting
    color_count = fix_json_file(input_file, fixed_file)
    
    if color_count == 0:
        print("No valid colors found!")
        return
    
    # Step 2: Convert to binary
    create_binary_database(fixed_file, binary_file)
    
    print(f"\nConversion complete!")
    print(f"Original file: {input_file}")
    print(f"Fixed JSON: {fixed_file}")
    print(f"Binary file: {binary_file}")

if __name__ == "__main__":
    main()
