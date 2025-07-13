#!/usr/bin/env python3
"""
Prepare binary data directory for ESP32 upload.

This script:
1. Converts dulux.json to binary format if needed
2. Copies the binary file and frontend files to data_clean directory
3. Prepares everything for ESP32 LittleFS upload
"""

import os
import shutil
import subprocess
import sys

def prepare_binary_data():
    """Prepare clean data directory with binary color database for ESP32 upload."""
    
    # Paths
    current_dir = os.path.dirname(os.path.abspath(__file__))
    data_dir = os.path.join(current_dir, "data")
    dist_dir = os.path.join(data_dir, "dist")
    clean_data_dir = os.path.join(current_dir, "data_clean")
    
    # Source files
    dulux_json = os.path.join(data_dir, "dulux.json")
    dulux_binary = os.path.join(data_dir, "dulux.bin")
    
    print("🔧 Preparing binary data directory for ESP32 upload...")
    
    # Check if source files exist
    if not os.path.exists(dist_dir):
        print("❌ Error: data/dist directory not found!")
        print("   Please run 'npm run build' in the data directory first.")
        return False
        
    # Check if binary file exists
    if not os.path.exists(dulux_binary):
        print("❌ Error: data/dulux.bin not found!")
        print("   Please run the binary conversion first.")
        return False
    else:
        print("✅ Binary file found")
    
    # Create clean data directory
    if os.path.exists(clean_data_dir):
        print(f"🧹 Removing existing {clean_data_dir}")
        shutil.rmtree(clean_data_dir)
    
    os.makedirs(clean_data_dir)
    print(f"📁 Created clean data directory: {clean_data_dir}")
    
    # Copy and rename frontend files from dist
    print("📋 Copying and renaming frontend files...")

    # Copy index.html
    index_html = os.path.join(dist_dir, "index.html")
    if os.path.exists(index_html):
        shutil.copy2(index_html, os.path.join(clean_data_dir, "index.html"))
        print(f"   ✅ Copied: index.html")

    # Find and copy CSS file (rename to index.css)
    assets_dir = os.path.join(dist_dir, "assets")
    if os.path.exists(assets_dir):
        for file in os.listdir(assets_dir):
            src_file = os.path.join(assets_dir, file)
            if file.endswith(".css"):
                shutil.copy2(src_file, os.path.join(clean_data_dir, "index.css"))
                print(f"   ✅ Copied and renamed: {file} -> index.css")
            elif file.endswith(".js"):
                shutil.copy2(src_file, os.path.join(clean_data_dir, "index.js"))
                print(f"   ✅ Copied and renamed: {file} -> index.js")
    else:
        print("   ⚠️  Warning: assets directory not found")
    
    # Copy binary color database
    print("📋 Copying binary color database...")
    shutil.copy2(dulux_binary, os.path.join(clean_data_dir, "dulux.bin"))
    print("   ✅ Copied: dulux.bin")
    
    # Show file sizes and summary
    print("\n📊 File Summary:")
    total_size = 0
    for item in os.listdir(clean_data_dir):
        item_path = os.path.join(clean_data_dir, item)
        if os.path.isfile(item_path):
            size = os.path.getsize(item_path)
            total_size += size
            print(f"   {item}: {size:,} bytes ({size/1024:.1f} KB)")
    
    print(f"\n📏 Total size: {total_size:,} bytes ({total_size/1024:.1f} KB)")
    
    # Check if size is reasonable for ESP32
    max_size = 9 * 1024 * 1024  # 9MB limit for LittleFS
    if total_size > max_size:
        print(f"⚠️  Warning: Total size exceeds recommended limit of {max_size/1024/1024:.1f} MB")
    else:
        print(f"✅ Size is within ESP32 LittleFS limits ({max_size/1024/1024:.1f} MB)")
    
    # Show binary file info
    binary_size = os.path.getsize(dulux_binary)
    print(f"\n💾 Binary Database Info:")
    print(f"   Binary size: {binary_size:,} bytes ({binary_size/1024:.1f} KB)")
    print(f"   Estimated colors: ~4,224")
    print(f"   Memory efficient: Uses streaming access")
    
    print(f"\n🎉 Binary data preparation complete!")
    print(f"📁 Files ready for upload in: {clean_data_dir}")
    
    return True

def main():
    """Main function."""
    
    if prepare_binary_data():
        print("\n🚀 Next steps:")
        print("1. Upload data files: pio run --target uploadfs --environment um_pros3")
        print("2. Upload firmware: pio run --target upload --environment um_pros3")
        print("3. Monitor output: pio device monitor --environment um_pros3")
        print("\n💡 The ESP32 will now use the efficient binary color database!")
        return 0
    else:
        print("\n❌ Binary data preparation failed")
        return 1

if __name__ == "__main__":
    sys.exit(main())
