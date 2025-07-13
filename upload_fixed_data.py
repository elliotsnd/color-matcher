#!/usr/bin/env python3
"""
Upload the corrected data files to ESP32.
This script handles the directory swapping needed for PlatformIO.
"""

import os
import shutil
import subprocess
import sys

def upload_fixed_data():
    """Upload the fixed data files to ESP32."""
    
    print("🚀 Uploading fixed data files to ESP32...")
    
    # Check if data_clean exists
    if not os.path.exists("data_clean"):
        print("❌ Error: data_clean directory not found")
        print("   Please run 'python prepare_binary_data.py' first")
        return False
    
    # Check if original data directory exists
    data_backup_needed = os.path.exists("data")
    
    try:
        # Step 1: Backup original data directory if it exists
        if data_backup_needed:
            print("📦 Backing up original data directory...")
            if os.path.exists("data_backup"):
                shutil.rmtree("data_backup")
            shutil.move("data", "data_backup")
        
        # Step 2: Move data_clean to data
        print("🔄 Preparing data directory for upload...")
        shutil.move("data_clean", "data")
        
        # Step 3: Upload filesystem
        print("📤 Uploading filesystem to ESP32...")
        result = subprocess.run(['pio', 'run', '--target', 'uploadfs', '--environment', 'um_pros3'], 
                              capture_output=True, text=True)
        
        if result.returncode == 0:
            print("✅ Filesystem uploaded successfully!")
            upload_success = True
        else:
            print(f"❌ Upload failed: {result.stderr}")
            upload_success = False
        
        # Step 4: Restore directory structure
        print("🔄 Restoring directory structure...")
        shutil.move("data", "data_clean")
        
        if data_backup_needed:
            shutil.move("data_backup", "data")
        
        if upload_success:
            print("\n🎉 Upload completed successfully!")
            print("📁 Files uploaded:")
            print("   • index.html - Web interface")
            print("   • index.css - Styles")
            print("   • index.js - JavaScript")
            print("   • dulux.bin - Binary color database")
            
            print("\n🌐 Next steps:")
            print("1. Reset the ESP32 or upload new firmware")
            print("2. Open http://192.168.0.152 in your browser")
            print("3. The web interface should now load correctly!")
            
            return True
        else:
            return False
            
    except Exception as e:
        print(f"❌ Error during upload process: {e}")
        
        # Try to restore directories in case of error
        try:
            if os.path.exists("data"):
                if os.path.exists("data_clean"):
                    shutil.rmtree("data")
                else:
                    shutil.move("data", "data_clean")
            
            if data_backup_needed and os.path.exists("data_backup"):
                shutil.move("data_backup", "data")
        except:
            print("⚠️  Warning: Could not restore directory structure")
            print("   Please manually check data/ and data_clean/ directories")
        
        return False

def main():
    """Main function."""
    
    if upload_fixed_data():
        print("\n🎯 Web interface should now be working!")
        print("   Try accessing: http://192.168.0.152")
        return 0
    else:
        print("\n❌ Upload failed")
        return 1

if __name__ == "__main__":
    sys.exit(main())
