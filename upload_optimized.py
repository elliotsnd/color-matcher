#!/usr/bin/env python3
"""
Optimized script to build and upload only essential files to ESP32-S3 LittleFS
This script builds the frontend and uploads only the compiled files + database
"""

import os
import shutil
import subprocess
import sys
import json
import tempfile

def run_command(cmd, cwd=None, check=True):
    """Run a command and return the result"""
    try:
        result = subprocess.run(cmd, shell=True, cwd=cwd, check=check, 
                              capture_output=True, text=True)
        return result
    except subprocess.CalledProcessError as e:
        print(f"❌ Command failed: {cmd}")
        print(f"Error: {e.stderr}")
        raise

def build_frontend():
    """Build the React TypeScript frontend"""
    print("🔨 Building frontend...")
    
    data_dir = "data"
    if not os.path.exists(data_dir):
        print(f"❌ Error: {data_dir} directory not found")
        return False
    
    # Check if package.json exists
    package_json = os.path.join(data_dir, "package.json")
    if not os.path.exists(package_json):
        print(f"❌ Error: {package_json} not found")
        return False
    
    # Install dependencies if node_modules doesn't exist
    node_modules = os.path.join(data_dir, "node_modules")
    if not os.path.exists(node_modules):
        print("📦 Installing dependencies...")
        run_command("npm install", cwd=data_dir)
    
    # Build the project
    print("⚙️  Building project...")
    result = run_command("npm run build", cwd=data_dir)
    
    # Check if dist directory was created
    dist_dir = os.path.join(data_dir, "dist")
    if not os.path.exists(dist_dir):
        print("❌ Error: Build failed - dist directory not created")
        return False
    
    print("✅ Frontend built successfully")
    return True

def prepare_upload_files():
    """Prepare only essential files for upload"""
    print("📁 Preparing files for upload...")
    
    # Create temporary directory for upload files
    temp_dir = tempfile.mkdtemp(prefix="esp32_upload_")
    print(f"📂 Temporary upload directory: {temp_dir}")
    
    try:
        # Copy built frontend files
        dist_dir = os.path.join("data", "dist")
        if os.path.exists(dist_dir):
            for file in os.listdir(dist_dir):
                src = os.path.join(dist_dir, file)
                if os.path.isfile(src):
                    # Rename files to expected names
                    if file == "index.html":
                        dst = os.path.join(temp_dir, "index.html")
                    elif file.endswith(".css"):
                        dst = os.path.join(temp_dir, "index.css")
                    elif file.endswith(".js"):
                        dst = os.path.join(temp_dir, "index.js")
                    else:
                        continue  # Skip other files
                    
                    shutil.copy2(src, dst)
                    size = os.path.getsize(dst)
                    print(f"✅ {os.path.basename(dst)}: {size:,} bytes")
        
        # Copy assets directory if it exists
        assets_src = os.path.join(dist_dir, "assets")
        if os.path.exists(assets_src):
            assets_dst = os.path.join(temp_dir, "assets")
            shutil.copytree(assets_src, assets_dst)
            
            # Rename CSS and JS files in assets to expected names
            for file in os.listdir(assets_dst):
                if file.endswith(".css"):
                    old_path = os.path.join(assets_dst, file)
                    new_path = os.path.join(temp_dir, "index.css")
                    shutil.move(old_path, new_path)
                    size = os.path.getsize(new_path)
                    print(f"✅ index.css: {size:,} bytes")
                elif file.endswith(".js"):
                    old_path = os.path.join(assets_dst, file)
                    new_path = os.path.join(temp_dir, "index.js")
                    shutil.move(old_path, new_path)
                    size = os.path.getsize(new_path)
                    print(f"✅ index.js: {size:,} bytes")
            
            # Remove empty assets directory
            if os.path.exists(assets_dst) and not os.listdir(assets_dst):
                os.rmdir(assets_dst)
        
        # Copy essential data files
        essential_files = ["dulux.json", "metadata.json"]
        for file in essential_files:
            src = os.path.join("data", file)
            if os.path.exists(src):
                dst = os.path.join(temp_dir, file)
                shutil.copy2(src, dst)
                size = os.path.getsize(dst)
                print(f"✅ {file}: {size:,} bytes")
        
        # Calculate total size
        total_size = sum(os.path.getsize(os.path.join(temp_dir, f)) 
                        for f in os.listdir(temp_dir) 
                        if os.path.isfile(os.path.join(temp_dir, f)))
        print(f"📊 Total upload size: {total_size:,} bytes ({total_size/1024:.1f} KB)")
        
        # Check size limit
        if total_size > 5 * 1024 * 1024:  # 5MB limit
            print("⚠️  Warning: Upload size is large, may not fit in LittleFS")
        
        return temp_dir
        
    except Exception as e:
        print(f"❌ Error preparing files: {e}")
        shutil.rmtree(temp_dir, ignore_errors=True)
        return None

def upload_to_esp32(upload_dir):
    """Upload files to ESP32 using PlatformIO"""
    print("🚀 Uploading to ESP32...")
    
    # Backup original data directory
    data_backup = "data_backup"
    if os.path.exists("data"):
        if os.path.exists(data_backup):
            shutil.rmtree(data_backup)
        shutil.move("data", data_backup)
        print("📦 Backed up original data directory")
    
    try:
        # Create new data directory with only upload files
        shutil.copytree(upload_dir, "data")
        
        # Upload filesystem
        result = run_command("pio run --target uploadfs --environment um_pros3")
        
        if result.returncode == 0:
            print("✅ Files uploaded successfully to ESP32!")
            return True
        else:
            print("❌ Upload failed")
            return False
            
    finally:
        # Restore original data directory
        if os.path.exists("data"):
            shutil.rmtree("data")
        if os.path.exists(data_backup):
            shutil.move(data_backup, "data")
            print("📦 Restored original data directory")

def main():
    print("ESP32-S3 Optimized Upload Tool")
    print("=" * 40)
    
    # Check PlatformIO
    try:
        result = run_command("pio --version", check=False)
        if result.returncode != 0:
            raise FileNotFoundError
        print(f"📦 PlatformIO: {result.stdout.strip()}")
    except FileNotFoundError:
        print("❌ PlatformIO not found. Install from: https://platformio.org/install/cli")
        sys.exit(1)
    
    # Check project directory
    if not os.path.exists('platformio.ini'):
        print("❌ Run this script from the project root directory")
        sys.exit(1)
    
    try:
        # Step 1: Build frontend
        if not build_frontend():
            sys.exit(1)
        
        # Step 2: Prepare upload files
        upload_dir = prepare_upload_files()
        if not upload_dir:
            sys.exit(1)
        
        # Step 3: Upload to ESP32
        success = upload_to_esp32(upload_dir)
        
        # Cleanup
        shutil.rmtree(upload_dir, ignore_errors=True)
        
        if success:
            print("\n🎉 Upload completed successfully!")
            print("\n🔧 Next steps:")
            print("1. Upload firmware: pio run --target upload")
            print("2. Monitor: pio device monitor")
            print("3. Access web interface at device IP")
        else:
            sys.exit(1)
            
    except KeyboardInterrupt:
        print("\n❌ Upload cancelled by user")
        sys.exit(1)
    except Exception as e:
        print(f"❌ Unexpected error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
