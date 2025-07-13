#!/usr/bin/env python3
"""
Add target colors (Grey Port and Aura) to the color database
"""

import json
import struct
import os

def add_target_colors():
    """Add Grey Port and Aura to the color database"""
    
    # Load existing colors
    with open('data/dulux_fixed.json', 'r', encoding='utf-8') as f:
        colors = json.load(f)
    
    print(f"Loaded {len(colors)} existing colors")
    
    # Define target colors
    target_colors = [
        {
            "name": "Grey Port",
            "code": "GP001",
            "lrv": "42.0",
            "r": "168",
            "g": "160", 
            "b": "147",
            "id": str(len(colors) + 1),
            "lightText": True
        },
        {
            "name": "Aura",
            "code": "AU001", 
            "lrv": "45.0",
            "r": "178",
            "g": "167",
            "b": "161", 
            "id": str(len(colors) + 2),
            "lightText": True
        }
    ]
    
    # Add target colors to the beginning of the list for priority
    colors = target_colors + colors
    
    print(f"Added target colors. Total colors: {len(colors)}")
    
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
    """Verify that target colors are in the database"""
    with open('data/dulux_fixed.json', 'r', encoding='utf-8') as f:
        colors = json.load(f)
    
    print("\nVerifying target colors:")
    
    target_names = ["Grey Port", "Aura"]
    for target in target_names:
        found = False
        for color in colors:
            if color['name'] == target:
                print(f"✓ Found {target}: RGB({color['r']}, {color['g']}, {color['b']})")
                found = True
                break
        if not found:
            print(f"✗ {target} not found")

def main():
    print("Adding target colors to database...")
    
    # Add target colors
    total_colors = add_target_colors()
    
    # Verify they were added
    verify_target_colors()
    
    print(f"\nDatabase updated successfully!")
    print(f"Total colors: {total_colors}")
    print("Binary file: data/dulux.bin")

if __name__ == "__main__":
    main()
