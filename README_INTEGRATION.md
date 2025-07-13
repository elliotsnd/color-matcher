# Color Sensor Web Interface Integration

This project integrates the Arduino color sensor with a React web frontend, allowing you to view real-time color data through a web browser.

## Hardware Requirements

- ESP8266 NodeMCU board
- TCS3430 color sensor
- LED (connected to pin 13)
- WiFi network

## Software Requirements

- PlatformIO (for Arduino development)
- Python 3.x (for upload script)

## Setup Instructions

### 1. Configure WiFi Credentials

Edit `src/main.cpp` and update the WiFi credentials:

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

### 2. Upload Frontend Files to ESP8266

The frontend files need to be uploaded to the ESP8266's LittleFS filesystem:

```bash
# Make the upload script executable (Linux/Mac)
chmod +x upload_data.py

# Run the upload script
python upload_data.py
```

Or manually using PlatformIO:
```bash
pio run --target uploadfs
```

### 3. Compile and Upload Arduino Code

```bash
pio run --target upload
```

### 4. Monitor Serial Output

```bash
pio device monitor
```

Look for the IP address in the serial output. It will show something like:
```
WiFi connected! IP address: 192.168.1.100
Web server started
```

### 5. Access the Web Interface

Open your web browser and navigate to the IP address shown in the serial monitor (e.g., `http://192.168.1.100`).

## Features

### Real-time Color Display
- Live view of color sensor readings
- Automatic color name generation using AI
- RGB and HEX color values

### Color Capture
- Capture current color reading
- Save colors to local storage
- View saved color history

### API Endpoints

The ESP8266 serves the following endpoints:

- `GET /` - Main web interface
- `GET /index.css` - Stylesheet
- `GET /index.tsx` - JavaScript application
- `GET /api/color` - JSON API for current color data

### API Response Format

```json
{
  "r": 255,
  "g": 128,
  "b": 64,
  "x": 12345,
  "y": 23456,
  "z": 34567,
  "ir1": 1234,
  "ir2": 2345,
  "timestamp": 123456789
}
```

## Calibration

The system uses the final calibrated parameters from your color sensor testing:

- IR Compensation: 0.5
- Red Channel: slope=0.01180, offset=52.28
- Green Channel: slope=0.01359, offset=24.06  
- Blue Channel: slope=0.01904, offset=79.37

## Troubleshooting

### WiFi Connection Issues
- Check SSID and password in `src/main.cpp`
- Ensure ESP8266 is in range of WiFi network
- Check serial monitor for connection status

### File Upload Issues
- Ensure PlatformIO is installed and in PATH
- Check that `data` folder contains all frontend files
- Try manual upload: `pio run --target uploadfs`

### Sensor Issues
- Check I2C connections (SDA/SCL)
- Verify sensor power supply
- Check serial monitor for sensor initialization messages

### Web Interface Issues
- Verify ESP8266 IP address from serial monitor
- Check that all files uploaded successfully
- Try refreshing the browser page
- Check browser console for JavaScript errors

## File Structure

```
├── src/
│   └── main.cpp              # Arduino code with web server
├── data/                     # Frontend files (uploaded to ESP8266)
│   ├── index.html           # Main HTML page
│   ├── index.css            # Styles
│   ├── index.tsx            # React application
│   └── package.json         # Dependencies info
├── platformio.ini           # PlatformIO configuration
├── upload_data.py           # Upload script
└── README_INTEGRATION.md    # This file
```

## Development

To modify the frontend:
1. Edit files in the `data/` folder
2. Re-upload using `python upload_data.py`
3. Refresh browser to see changes

To modify the Arduino code:
1. Edit `src/main.cpp`
2. Upload using `pio run --target upload`
3. Monitor serial output for debugging
