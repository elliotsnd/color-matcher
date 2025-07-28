#!/usr/bin/env python3
import json
import sys

def process_color_data(data):
    """Process color data from ESP32"""
    # Simple color processing logic
    result = {
        "status": "success",
        "processed": True,
        "rgb": data.get("rgb", [0, 0, 0]),
        "timestamp": data.get("timestamp", "")
    }
    return result

if __name__ == "__main__":
    try:
        input_data = json.loads(sys.stdin.read())
        result = process_color_data(input_data)
        print(json.dumps(result))
    except Exception as e:
        print(json.dumps({"error": str(e), "status": "failed"}))
