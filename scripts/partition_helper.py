#!/usr/bin/env python3

"""
Partition helper script for ESP32-S3 with 16MB flash and LittleFS
"""

try:
    Import("env")
except:
    pass

def print_partition_info():
    print("=" * 50)
    print("ESP32-S3 16MB Flash Partition Layout")
    print("=" * 50)
    print("NVS:      0x009000 - 0x00E000 (20KB)")
    print("OTA Data: 0x00E000 - 0x010000 (8KB)")
    print("App 0:    0x010000 - 0x330000 (3200KB)")
    print("App 1:    0x330000 - 0x650000 (3200KB)")
    print("LittleFS: 0x650000 - 0x1000000 (10112KB)")
    print("=" * 50)

# Print partition info during build
print_partition_info()
