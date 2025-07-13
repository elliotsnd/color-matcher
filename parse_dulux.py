#!/usr/bin/env python3
import re

def parse_dulux_bin():
    """Parse the dulux.bin file to extract color names and RGB values"""
    try:
        with open('data/dulux.bin', 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
        
        # Based on the sample data, let me try a different approach
        # Looking at: "Vivid WhiteW01A190.00100001"
        # It seems like: Name + Code + Lightness + RGB as 6 digits
        
        # Let's first examine some raw data
        print("Sample data from file:")
        print(repr(content[:200]))
        print("\n" + "="*50 + "\n")
        
        # Split by null characters or other separators
        entries = []
        current_entry = ""
        
        for char in content:
            if ord(char) < 32 and char not in ['\n', '\r', '\t']:  # Control character (likely separator)
                if current_entry.strip():
                    entries.append(current_entry.strip())
                current_entry = ""
            else:
                current_entry += char
        
        if current_entry.strip():
            entries.append(current_entry.strip())
        
        print(f"Found {len(entries)} entries")
        print("\nFirst 10 entries:")
        for i, entry in enumerate(entries[:10]):
            print(f"{i+1}. {repr(entry)}")
        
        # Now try to parse each entry
        colors = []
        pattern = r'^([A-Za-z\s\'\.\-&]+?)([A-Z]\d+[A-Z]?\d*[A-Z]?)(\d+\.\d+)(\d{6})$'
        
        for entry in entries:
            match = re.match(pattern, entry)
            if match:
                color_name = match.group(1).strip()
                code = match.group(2)
                lightness = float(match.group(3))
                rgb_value = match.group(4)
                
                # Parse RGB - might need to interpret this differently
                if len(rgb_value) == 6:
                    try:
                        # Try as hex first
                        r = int(rgb_value[:2], 16) if rgb_value[:2].isdigit() else int(rgb_value[:2])
                        g = int(rgb_value[2:4], 16) if rgb_value[2:4].isdigit() else int(rgb_value[2:4])
                        b = int(rgb_value[4:6], 16) if rgb_value[4:6].isdigit() else int(rgb_value[4:6])
                        
                        colors.append({
                            'name': color_name,
                            'code': code,
                            'r': r,
                            'g': g,
                            'b': b,
                            'lightness': lightness,
                            'raw_rgb': rgb_value
                        })
                    except ValueError:
                        # Try as decimal
                        try:
                            r = int(rgb_value[:2])
                            g = int(rgb_value[2:4])
                            b = int(rgb_value[4:6])
                            
                            colors.append({
                                'name': color_name,
                                'code': code,
                                'r': r,
                                'g': g,
                                'b': b,
                                'lightness': lightness,
                                'raw_rgb': rgb_value
                            })
                        except ValueError:
                            continue
        
        print(f"\nSuccessfully parsed {len(colors)} colors")
        if colors:
            print("\nFirst 5 parsed colors:")
            for i, color in enumerate(colors[:5]):
                print(f"{i+1}. {color['name']} - R:{color['r']} G:{color['g']} B:{color['b']} (Raw: {color['raw_rgb']})")
            
        return colors
        
    except Exception as e:
        print(f"Error parsing file: {e}")
        import traceback
        traceback.print_exc()
        return []

if __name__ == "__main__":
    colors = parse_dulux_bin()
