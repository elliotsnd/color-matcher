#!/usr/bin/env python3
"""
Generate cross-platform compatible compile_commands.json for static analysis tools
This script creates a compile commands database that works in both Windows and Linux environments
"""

import json
import os
import sys

def get_library_includes(project_root):
    """Find all library include paths."""
    includes = []
    libdeps_dir = os.path.join(project_root, ".pio", "libdeps", "um_pros3")
    if os.path.isdir(libdeps_dir):
        for lib_dir in os.listdir(libdeps_dir):
            lib_path = os.path.join(libdeps_dir, lib_dir)
            if os.path.isdir(lib_path):
                # Common src directory
                src_path = os.path.join(lib_path, "src")
                if os.path.isdir(src_path):
                    includes.append(f"-I{src_path.replace(' ', ' ')}")
                # Root library directory
                includes.append(f"-I{lib_path.replace(' ', ' ')}")
    return includes

def create_compile_commands():
    """Create a basic compile commands database for the color sensor project"""
    
    # Project root directory
    project_root = os.path.dirname(os.path.abspath(__file__))
    
    library_includes = get_library_includes(project_root)
    
    base_flags = [
        "clang++",
        "-std=c++17",
        "-DARDUINO=10819",
        "-DARDUINO_ESP32_DEV=1",
        "-DESP32=1",
        "-DESP_PLATFORM=1",
        "-DF_CPU=240000000L",
        "-DBOARD_HAS_PSRAM=1",
        "-DCONFIG_LITTLEFS_ENABLE=1",
        "-DCONFIG_FATFS_ENABLE=0",
        "-DCONFIG_SPIFFS_ENABLE=0",
        "-DPROGMEM=",
        "-Isrc",
        "-Ilib",
        "-Iinclude"
    ] + library_includes

    # Basic compile command for the main source file
    compile_commands = []
    
    # Add all source files found in src directory
    src_dir = os.path.join(project_root, "src")
    if os.path.exists(src_dir):
        for file in os.listdir(src_dir):
            if file.endswith(('.cpp', '.c', '.cxx')):
                file_path = os.path.join(src_dir, file).replace('\\', '/')
                command = base_flags + [file_path]
                compile_commands.append({
                    "directory": project_root,
                    "command": " ".join(command),
                    "file": file_path
                })
    
    # Write to compile_commands.json
    output_file = os.path.join(project_root, "compile_commands.json")
    with open(output_file, 'w') as f:
        json.dump(compile_commands, f, indent=2)
    
    print(f"✅ Generated cross-platform compile_commands.json with {len(compile_commands)} entries")
    print(f"📄 Output: {output_file}")
    
    return output_file

if __name__ == "__main__":
    create_compile_commands()