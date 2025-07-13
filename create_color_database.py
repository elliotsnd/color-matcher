#!/usr/bin/env python3
"""
Convert Dulux JSON color database to compact binary format for ESP32
Binary format: Each color entry is 64 bytes:
- 32 bytes: Color name (null-terminated string)
- 16 bytes: Color code (null-terminated string) 
- 3 bytes: RGB values (r, g, b)
- 4 bytes: ID (uint32_t)
- 4 bytes: LRV (float)
- 5 bytes: Reserved for alignment
"""

import json
import struct
import os

def create_binary_database(json_file, output_file):
    """Convert JSON color database to binary format"""
    
    # Load the complete JSON database
    with open(json_file, 'r') as f:
        colors = json.load(f)
    
    print(f"Loading {len(colors)} colors from {json_file}")
    
    # Binary format: 64 bytes per color
    ENTRY_SIZE = 64
    NAME_SIZE = 32
    CODE_SIZE = 16
    
    with open(output_file, 'wb') as f:
        # Write header: 4 bytes for count
        f.write(struct.pack('<I', len(colors)))
        
        for i, color in enumerate(colors):
            # Extract color data
            name = color['name'][:NAME_SIZE-1]  # Truncate if too long
            code = color['code'][:CODE_SIZE-1]
            r = int(color['r'])
            g = int(color['g']) 
            b = int(color['b'])
            color_id = int(color['id'])
            lrv = float(color['lrv'])
            
            # Create 64-byte entry
            entry = bytearray(ENTRY_SIZE)
            
            # Color name (32 bytes, null-terminated)
            name_bytes = name.encode('utf-8')[:NAME_SIZE-1]
            entry[0:len(name_bytes)] = name_bytes
            entry[len(name_bytes)] = 0  # Null terminator
            
            # Color code (16 bytes, null-terminated) 
            code_bytes = code.encode('utf-8')[:CODE_SIZE-1]
            entry[NAME_SIZE:NAME_SIZE+len(code_bytes)] = code_bytes
            entry[NAME_SIZE+len(code_bytes)] = 0  # Null terminator
            
            # RGB values (3 bytes)
            entry[48] = r
            entry[49] = g  
            entry[50] = b
            
            # ID (4 bytes, little-endian uint32)
            struct.pack_into('<I', entry, 52, color_id)
            
            # LRV (4 bytes, little-endian float)
            struct.pack_into('<f', entry, 56, lrv)
            
            # Remaining 4 bytes are reserved/padding
            
            f.write(entry)
            
            if i % 100 == 0:
                print(f"Processed {i+1}/{len(colors)} colors...")
    
    file_size = os.path.getsize(output_file)
    print(f"Created binary database: {output_file}")
    print(f"File size: {file_size:,} bytes ({file_size/1024:.1f} KB)")
    print(f"Colors: {len(colors)}")
    print(f"Bytes per color: {ENTRY_SIZE}")

def create_sample_database():
    """Create a small sample database for testing"""
    sample_colors = [
        {
            "name": "Vivid White",
            "code": "W01A1", 
            "r": "255",
            "g": "255",
            "b": "255",
            "id": "100001",
            "lrv": "90.00"
        },
        {
            "name": "Antique White USA",
            "code": "W01B1",
            "r": "245", 
            "g": "240",
            "b": "230",
            "id": "100002", 
            "lrv": "85.00"
        },
        {
            "name": "Natural White",
            "code": "W01D1",
            "r": "245",
            "g": "245", 
            "b": "235",
            "id": "100004",
            "lrv": "84.00"
        },
        {
            "name": "Whisper White", 
            "code": "W01E1",
            "r": "242",
            "g": "240",
            "b": "238", 
            "id": "100005",
            "lrv": "83.50"
        },
        {
            "name": "Rock Salt",
            "code": "W01G1", 
            "r": "248",
            "g": "245",
            "b": "240",
            "id": "100007",
            "lrv": "86.00"
        }
    ]
    
    # Save sample as JSON first
    with open('data/dulux_sample.json', 'w') as f:
        json.dump(sample_colors, f, indent=2)
    
    # Create binary version
    create_binary_database('data/dulux_sample.json', 'data/colors.bin')

if __name__ == "__main__":
    # Check if full database exists, otherwise create sample
    if os.path.exists('data/dulux_colors.json'):
        print("Using existing dulux_colors.json")
        create_binary_database('data/dulux_colors.json', 'data/colors.bin')
    else:
        print("Creating sample database for testing...")
        create_sample_database()
