#!/usr/bin/env python3
"""
Fix JSON formatting issues in dulux.json file
"""

import json
import re
import os

def fix_dulux_json(input_file, output_file):
    """Fix the malformed JSON file"""
    print(f"Fixing JSON formatting in: {input_file}")
    
    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    print(f"Original file size: {len(content)} characters")
    
    # Remove any existing array brackets
    content = content.strip()
    if content.startswith('['):
        content = content[1:]
    if content.endswith(']'):
        content = content[:-1]
    
    # Split by lines and process each line
    lines = content.split('\n')
    
    # Find complete JSON objects
    objects = []
    current_object = []
    brace_count = 0
    
    for line_num, line in enumerate(lines):
        line = line.strip()
        if not line:
            continue
            
        # Count braces to track object boundaries
        for char in line:
            if char == '{':
                brace_count += 1
            elif char == '}':
                brace_count -= 1
        
        current_object.append(line)
        
        # When we complete an object (brace_count returns to 0)
        if brace_count == 0 and current_object:
            obj_text = '\n'.join(current_object)
            
            # Clean up the object text
            obj_text = obj_text.strip()
            if obj_text.endswith(','):
                obj_text = obj_text[:-1]  # Remove trailing comma
            
            try:
                # Try to parse the object
                parsed_obj = json.loads(obj_text)
                
                # Validate required fields
                if 'name' in parsed_obj and 'r' in parsed_obj and 'g' in parsed_obj and 'b' in parsed_obj:
                    # Ensure all required fields are present
                    if 'code' not in parsed_obj or parsed_obj['code'] == 'N/A':
                        parsed_obj['code'] = f"C{len(objects)+1:05d}"
                    if 'lrv' not in parsed_obj or parsed_obj['lrv'] == 'N/A':
                        parsed_obj['lrv'] = "50.0"
                    if 'id' not in parsed_obj:
                        parsed_obj['id'] = str(len(objects) + 1)
                    if 'lightText' not in parsed_obj:
                        # Calculate if light text is needed
                        try:
                            r, g, b = int(parsed_obj['r']), int(parsed_obj['g']), int(parsed_obj['b'])
                            luminance = (0.299 * r + 0.587 * g + 0.114 * b)
                            parsed_obj['lightText'] = luminance < 128
                        except:
                            parsed_obj['lightText'] = False
                    
                    objects.append(parsed_obj)
                    
                    if len(objects) % 100 == 0:
                        print(f"Processed {len(objects)} valid objects...")
                        
            except json.JSONDecodeError as e:
                print(f"Skipping invalid object at line {line_num}: {e}")
            
            # Reset for next object
            current_object = []
            brace_count = 0
    
    print(f"Found {len(objects)} valid color objects")
    
    # Write the fixed JSON
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(objects, f, indent=2, ensure_ascii=False)
    
    print(f"Fixed JSON saved to: {output_file}")
    return len(objects)

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
        import struct
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
    print("Step 1: Fixing JSON formatting...")
    color_count = fix_dulux_json(input_file, fixed_file)
    
    if color_count == 0:
        print("No valid colors found!")
        return
    
    # Step 2: Convert to binary
    print("\nStep 2: Converting to binary format...")
    create_binary_database(fixed_file, binary_file)
    
    print(f"\nConversion complete!")
    print(f"Original file: {input_file}")
    print(f"Fixed JSON: {fixed_file}")
    print(f"Binary file: {binary_file}")
    
    # Step 3: Search for specific colors
    print(f"\nStep 3: Searching for target colors...")
    with open(fixed_file, 'r', encoding='utf-8') as f:
        colors = json.load(f)
    
    # Search for colors close to Grey Port (168, 160, 147)
    target_r, target_g, target_b = 168, 160, 147
    close_matches = []
    
    for color in colors:
        try:
            r, g, b = int(color['r']), int(color['g']), int(color['b'])
            distance = ((r - target_r)**2 + (g - target_g)**2 + (b - target_b)**2)**0.5
            if distance < 10:  # Within 10 units
                close_matches.append((color['name'], r, g, b, distance))
        except:
            continue
    
    close_matches.sort(key=lambda x: x[4])  # Sort by distance
    
    print(f"\nColors close to Grey Port (168, 160, 147):")
    for name, r, g, b, dist in close_matches[:10]:
        print(f"  {name}: RGB({r}, {g}, {b}) - distance: {dist:.2f}")

if __name__ == "__main__":
    main()
