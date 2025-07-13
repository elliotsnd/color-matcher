#!/usr/bin/env python3
"""
Add Grey Port and fix Vivid White RGB values in the color database
"""

import json
import struct
import os

def update_color_database():
    """Update the color database with correct target colors"""
    
    # Load existing colors
    with open('data/dulux_fixed.json', 'r', encoding='utf-8') as f:
        colors = json.load(f)
    
    print(f"Loaded {len(colors)} existing colors")
    
    # Check if Grey Port already exists
    grey_port_exists = False
    for color in colors:
        if color['name'] == 'Grey Port':
            grey_port_exists = True
            print("Grey Port already exists in database")
            break
    
    # Add Grey Port if it doesn't exist
    if not grey_port_exists:
        grey_port = {
            "name": "Grey Port",
            "code": "GP001",
            "lrv": "42.0",
            "r": "168",
            "g": "160", 
            "b": "147",
            "id": str(len(colors) + 1),
            "lightText": True
        }
        colors.insert(0, grey_port)  # Add at beginning for priority
        print("Added Grey Port to database")
    
    # Find and update Vivid White
    vivid_white_found = False
    for i, color in enumerate(colors):
        if color['name'] == 'Vivid White':
            old_rgb = f"RGB({color['r']}, {color['g']}, {color['b']})"
            colors[i]['r'] = "247"
            colors[i]['g'] = "248" 
            colors[i]['b'] = "244"
            new_rgb = f"RGB({color['r']}, {color['g']}, {color['b']})"
            print(f"Updated Vivid White: {old_rgb} -> {new_rgb}")
            vivid_white_found = True
            break
    
    if not vivid_white_found:
        # Add Vivid White if it doesn't exist
        vivid_white = {
            "name": "Vivid White",
            "code": "SW1G1",
            "lrv": "94.0",
            "r": "247",
            "g": "248",
            "b": "244",
            "id": str(len(colors) + 1),
            "lightText": False
        }
        colors.insert(1, vivid_white)  # Add near beginning for priority
        print("Added Vivid White to database")
    
    print(f"Total colors: {len(colors)}")
    
    # Save updated JSON
    with open('data/dulux_fixed.json', 'w', encoding='utf-8') as f:
        json.dump(colors, f, indent=2, ensure_ascii=False)
    
    print("Updated JSON saved")
    
    # Create new binary database
    create_binary_database('data/dulux_fixed.json', 'data/dulux.bin')
    
    return len(colors)

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

def verify_target_colors():
    """Verify that target colors have correct RGB values"""
    with open('data/dulux_fixed.json', 'r', encoding='utf-8') as f:
        colors = json.load(f)
    
    print("\nVerifying target colors:")
    
    targets = {
        "Grey Port": (168, 160, 147),
        "Vivid White": (247, 248, 244)
    }
    
    for target_name, expected_rgb in targets.items():
        found = False
        for color in colors:
            if color['name'] == target_name:
                actual_rgb = (int(color['r']), int(color['g']), int(color['b']))
                if actual_rgb == expected_rgb:
                    print(f"✓ {target_name}: RGB{actual_rgb} - CORRECT")
                else:
                    print(f"✗ {target_name}: RGB{actual_rgb} - Expected RGB{expected_rgb}")
                found = True
                break
        if not found:
            print(f"✗ {target_name} not found in database")

def main():
    print("Updating color database with target colors...")
    
    # Update the database
    total_colors = update_color_database()
    
    # Verify the updates
    verify_target_colors()
    
    print(f"\nDatabase updated successfully!")
    print(f"Total colors: {total_colors}")
    print("Files updated:")
    print("  - data/dulux_fixed.json")
    print("  - data/dulux.bin")

if __name__ == "__main__":
    main()
