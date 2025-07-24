#!/usr/bin/env python3
"""
Regenerate compile_commands.json for better SonarQube C++ analysis
This ensures the compilation database is current and complete.
"""

import os
import subprocess
import sys
import json

def regenerate_compile_commands():
    """Regenerate the compilation database using PlatformIO"""
    print("Regenerating compilation database for SonarQube analysis...")
    
    # Clean previous build
    print("Cleaning previous build...")
    result = subprocess.run(["pio", "run", "--target", "clean"], 
                          capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Warning: Clean failed: {result.stderr}")
    
    # Generate compilation database
    print("Generating compilation database...")
    result = subprocess.run(["pio", "run", "--target", "compiledb"], 
                          capture_output=True, text=True)
    
    if result.returncode != 0:
        print(f"Error generating compilation database: {result.stderr}")
        return False
    
    # Verify the file was created and is valid
    if os.path.exists("compile_commands.json"):
        try:
            with open("compile_commands.json", 'r') as f:
                data = json.load(f)
                print(f"✓ Compilation database generated successfully with {len(data)} entries")
                return True
        except json.JSONDecodeError:
            print("✗ Generated compilation database is invalid JSON")
            return False
    else:
        print("✗ Compilation database was not generated")
        return False

def main():
    """Main function"""
    if not os.path.exists("platformio.ini"):
        print("Error: This script must be run from the PlatformIO project root")
        sys.exit(1)
    
    success = regenerate_compile_commands()
    if success:
        print("\n✓ Compilation database ready for SonarQube analysis")
        print("Run 'sonar-scanner' to perform analysis with improved C++ detection")
    else:
        print("\n✗ Failed to generate compilation database")
        sys.exit(1)

if __name__ == "__main__":
    main()
