# Copilot Instructions for This Codebase

## Overview
This project is a hybrid hardware/software color sensor system with a web-based frontend and a microcontroller backend. It is designed to read color data from a TCS3430 sensor, calibrate it, and make the results available to a web UI for visualization and further use.

## Architecture
- **Backend (Microcontroller, C++):**
  - Located in `src/main.cpp`.
  - Reads XYZ and IR data from the TCS3430 sensor, applies calibration, and outputs both raw and calibrated RGB values.
  - Intended to be extended to serve data over HTTP (e.g., via ESP8266/ESP32 web server) for frontend consumption.
  - Serial output is used for debugging and development.

- **Frontend (Web, React/Vite):**
  - Located in the `data/` directory (`index.tsx`, `index.js`, `index.html`, `index.css`).
  - Displays color data, likely polling a backend endpoint (e.g., `/api/color`) for live updates.
  - Built with Vite and TypeScript; see `data/package.json` and `data/vite.config.ts`.

- **VS Code Extension (Optional/Experimental):**
  - `vscode-anthropic-completion/` is a separate VS Code extension for xAI Grok completions. Not directly related to the color sensor workflow.

## Developer Workflows
- **Build and Flash Microcontroller:**
  - Uses PlatformIO (`platformio.ini`).
  - Typical commands:
    - `pio run` — build firmware
    - `pio run --target upload` — upload to device
    - `pio device monitor` — view serial output

- **Run Frontend Locally:**
  - From `data/`:
    - `npm install` — install dependencies
    - `npm run dev` — start local dev server
  - The frontend expects a backend API (e.g., `/api/color`) to provide live color data.

- **Upload Frontend to Microcontroller (LittleFS):**
  - Use `upload_data.py` to upload static files in `data/` to the device's filesystem (for ESP8266/ESP32 web server use).

## Integration Pattern
- The backend should expose a REST API (e.g., `/api/color`) serving the latest calibrated color data as JSON.
- The frontend polls this endpoint to update the UI in real time.
- Static frontend files can be served from the microcontroller's filesystem (LittleFS/SPIFFS) for a self-contained device.

## Project-Specific Conventions
- **Calibration:** Calibration constants and logic are in `main.cpp` and should not be changed without revalidating results.
- **Frontend/Backend Contract:** The JSON structure for color data should include at least `r`, `g`, `b`, `x`, `y`, `z`, `ir1`, `ir2`, and `timestamp` fields.
- **No direct coupling:** The VS Code extension is not part of the color sensor pipeline.

## Key Files
- `src/main.cpp` — backend logic, sensor reading, calibration, (extend to web server)
- `data/index.tsx` — main frontend UI
- `data/package.json`, `data/vite.config.ts` — frontend build config
- `upload_data.py` — script to upload frontend to device
- `platformio.ini` — PlatformIO config for microcontroller

## Example: Backend API Response
```json
{
  "r": 123,
  "g": 45,
  "b": 67,
  "x": 1000,
  "y": 2000,
  "z": 1500,
  "ir1": 300,
  "ir2": 250,
  "timestamp": 12345678
}
```

## Troubleshooting
- If the frontend does not update, check that the backend is serving `/api/color` and that the device is on the same network as your browser.
- Use serial output for debugging sensor readings and calibration.

---

_If any section is unclear or incomplete, please provide feedback for further refinement._
