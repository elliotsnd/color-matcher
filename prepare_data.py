#!/usr/bin/env python3
"""
Prepare clean data directory for ESP32 upload.
This script creates a clean data directory with only the necessary files:
- Built frontend files from data/dist/
- dulux.json color database
"""

import os
import shutil
import sys

def prepare_clean_data():
    """Prepare clean data directory for ESP32 upload."""
    
    # Paths
    current_dir = os.path.dirname(os.path.abspath(__file__))
    data_dir = os.path.join(current_dir, "data")
    dist_dir = os.path.join(data_dir, "dist")
    clean_data_dir = os.path.join(current_dir, "data_clean")
    dulux_json = os.path.join(data_dir, "dulux.json")
    
    print("🧹 Preparing clean data directory for ESP32 upload...")
    
    # Check if source files exist
    if not os.path.exists(dist_dir):
        print("❌ Error: data/dist directory not found!")
        print("   Please run 'npm run build' in the data directory first.")
        return False
        
    if not os.path.exists(dulux_json):
        print("❌ Error: data/dulux.json not found!")
        return False
    
    # Remove existing clean data directory
    if os.path.exists(clean_data_dir):
        print(f"🗑️  Removing existing {clean_data_dir}")
        shutil.rmtree(clean_data_dir)
    
    # Create clean data directory
    print(f"📁 Creating clean data directory: {clean_data_dir}")
    os.makedirs(clean_data_dir)
    
    # Copy dist contents to root of clean data
    print("📋 Copying built frontend files...")
    for item in os.listdir(dist_dir):
        src = os.path.join(dist_dir, item)
        dst = os.path.join(clean_data_dir, item)
        if os.path.isdir(src):
            shutil.copytree(src, dst)
            print(f"   📁 {item}/")
        else:
            shutil.copy2(src, dst)
            print(f"   📄 {item}")
    
    # Copy dulux.json
    print("📋 Copying dulux.json...")
    shutil.copy2(dulux_json, clean_data_dir)
    print("   📄 dulux.json")
    
    # Calculate sizes
    total_size = 0
    file_count = 0
    for root, dirs, files in os.walk(clean_data_dir):
        for file in files:
            file_path = os.path.join(root, file)
            size = os.path.getsize(file_path)
            total_size += size
            file_count += 1
            rel_path = os.path.relpath(file_path, clean_data_dir)
            print(f"   📄 {rel_path} ({size:,} bytes)")
    
    print(f"\n✅ Clean data directory prepared successfully!")
    print(f"   📊 Total files: {file_count}")
    print(f"   📊 Total size: {total_size:,} bytes ({total_size/1024:.1f} KB)")
    print(f"   📁 Location: {clean_data_dir}")
    
    print(f"\n📝 Next steps:")
    print(f"   1. Temporarily rename current data directory:")
    print(f"      mv data data_backup")
    print(f"   2. Rename clean directory to data:")
    print(f"      mv data_clean data")
    print(f"   3. Upload filesystem:")
    print(f"      pio run -t uploadfs")
    print(f"   4. Restore original data directory:")
    print(f"      mv data data_clean && mv data_backup data")
    
    return True

if __name__ == "__main__":
    success = prepare_clean_data()
    sys.exit(0 if success else 1)
