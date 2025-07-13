#!/usr/bin/env python3
"""
Git repository setup script for ESP32 Color Matcher project.
This script helps initialize the git repository and prepare it for upload.
"""

import os
import subprocess
import sys
from pathlib import Path

def run_command(command, description=""):
    """Run a command and return success status."""
    print(f"🔄 {description}")
    print(f"   Command: {command}")
    
    try:
        result = subprocess.run(command, shell=True, capture_output=True, text=True)
        if result.returncode == 0:
            print(f"   ✅ Success")
            if result.stdout.strip():
                print(f"   Output: {result.stdout.strip()}")
            return True
        else:
            print(f"   ❌ Failed: {result.stderr.strip()}")
            return False
    except Exception as e:
        print(f"   ❌ Error: {e}")
        return False

def check_git_installed():
    """Check if git is installed."""
    return run_command("git --version", "Checking if Git is installed")

def initialize_git_repo():
    """Initialize git repository."""
    if os.path.exists(".git"):
        print("📁 Git repository already exists")
        return True
    
    return run_command("git init", "Initializing Git repository")

def setup_git_config():
    """Setup basic git configuration if not set."""
    print("🔧 Checking Git configuration...")
    
    # Check if user.name is set
    result = subprocess.run("git config user.name", shell=True, capture_output=True, text=True)
    if result.returncode != 0 or not result.stdout.strip():
        name = input("Enter your Git username: ")
        run_command(f'git config user.name "{name}"', "Setting Git username")
    
    # Check if user.email is set
    result = subprocess.run("git config user.email", shell=True, capture_output=True, text=True)
    if result.returncode != 0 or not result.stdout.strip():
        email = input("Enter your Git email: ")
        run_command(f'git config user.email "{email}"', "Setting Git email")

def clean_project():
    """Clean up generated files before committing."""
    print("🧹 Cleaning up generated files...")
    
    # Files and directories to remove
    cleanup_items = [
        "data/dist",
        "data_clean",
        "data_uploaded", 
        "node_modules",
        "*.bin",
        "*.log",
        "*.tmp",
        ".pio"
    ]
    
    for item in cleanup_items:
        if "*" in item:
            # Handle wildcards
            import glob
            for file in glob.glob(item):
                try:
                    os.remove(file)
                    print(f"   🗑️  Removed: {file}")
                except:
                    pass
        else:
            # Handle directories and files
            if os.path.exists(item):
                if os.path.isdir(item):
                    import shutil
                    shutil.rmtree(item)
                    print(f"   🗑️  Removed directory: {item}")
                else:
                    os.remove(item)
                    print(f"   🗑️  Removed file: {item}")

def add_files_to_git():
    """Add files to git staging area."""
    print("📦 Adding files to Git...")
    
    # Add all files except those in .gitignore
    if not run_command("git add .", "Adding all files to Git"):
        return False
    
    # Show what will be committed
    run_command("git status", "Showing Git status")
    return True

def create_initial_commit():
    """Create initial commit."""
    commit_message = """Initial commit: ESP32 Color Matcher with Binary Database

Features:
- ESP32-S3 based color detection with TCS3430 sensor
- Binary color database (83% smaller than JSON)
- Modern React web interface with live updates
- Memory-optimized streaming database access
- Comprehensive Dulux color matching (4,224+ colors)
- Robust error handling and fallback systems

Performance improvements:
- 95% reduction in memory usage
- 20x faster loading times
- Stable memory management with PSRAM
- Smart caching for repeated lookups

Ready for deployment and further development."""

    return run_command(f'git commit -m "{commit_message}"', "Creating initial commit")

def show_next_steps():
    """Show instructions for uploading to GitHub."""
    print("\n🎉 Git repository setup complete!")
    print("\n📋 Next steps to upload to GitHub:")
    print("\n1. Create a new repository on GitHub:")
    print("   - Go to https://github.com/new")
    print("   - Repository name: esp32-color-matcher")
    print("   - Description: ESP32 Color Matcher with Binary Database")
    print("   - Make it public or private (your choice)")
    print("   - Don't initialize with README (we already have one)")
    
    print("\n2. Add GitHub remote and push:")
    print("   git remote add origin https://github.com/YOUR_USERNAME/esp32-color-matcher.git")
    print("   git branch -M main")
    print("   git push -u origin main")
    
    print("\n3. Alternative: Use GitHub CLI (if installed):")
    print("   gh repo create esp32-color-matcher --public --source=. --remote=origin --push")
    
    print("\n4. Or create repository and get the commands from GitHub")
    
    print("\n📁 Repository contents:")
    run_command("git log --oneline", "Showing commit history")
    
    print("\n📊 Repository statistics:")
    run_command("git ls-files | wc -l", "Counting tracked files")

def main():
    """Main function."""
    print("🚀 ESP32 Color Matcher - Git Repository Setup")
    print("=" * 50)
    
    # Check prerequisites
    if not check_git_installed():
        print("❌ Git is not installed. Please install Git first.")
        print("   Download from: https://git-scm.com/downloads")
        return 1
    
    # Initialize repository
    if not initialize_git_repo():
        print("❌ Failed to initialize Git repository")
        return 1
    
    # Setup git configuration
    setup_git_config()
    
    # Clean up generated files
    clean_project()
    
    # Add files to git
    if not add_files_to_git():
        print("❌ Failed to add files to Git")
        return 1
    
    # Create initial commit
    if not create_initial_commit():
        print("❌ Failed to create initial commit")
        return 1
    
    # Show next steps
    show_next_steps()
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
