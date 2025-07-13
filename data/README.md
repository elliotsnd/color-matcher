# Color Matcher - ESP32 Color Sensor Web Interface

A modern React TypeScript web interface for the ESP32-based color sensor with Dulux color database integration. Features real-time color detection, server-side color naming, and a beautiful dark theme with purple accents.

## Features

- 🎨 **Real-time Color Detection**: Live color sensor feed with 500ms polling
- 🏷️ **Dulux Color Database**: Server-side color naming using comprehensive Dulux paint database
- 💾 **Color Storage**: Save and manage your favorite colors locally
- 🌙 **Dark Theme**: Beautiful dark interface with purple gradients and animations
- 📱 **Responsive Design**: Works on desktop and mobile devices
- ⚡ **Fast Performance**: No external AI dependencies, all processing on ESP32

## Architecture

### Frontend (React TypeScript)
- **Framework**: React 19 with TypeScript
- **Build Tool**: Vite for fast development and building
- **Styling**: CSS with custom properties and animations
- **Icons**: Font Awesome 6.4.0 via CDN
- **State Management**: React hooks for local state

### Backend (ESP32)
- **Hardware**: ESP32 with TCS3430 color sensor
- **Color Database**: Local Dulux paint color database
- **API**: RESTful endpoints for color data
- **Storage**: LittleFS for web files and color database

## API Endpoints

- `GET /` - Main web interface
- `GET /api/color` - JSON color data with RGB values and Dulux color name
- `GET /index.css` - Stylesheet
- `GET /index.tsx` - JavaScript application

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
  "colorName": "Dulux Warm Terracotta (N43-4)",
  "timestamp": 123456789
}
```

## Development Setup

**Prerequisites:** Node.js 16+

1. **Install dependencies:**
   ```bash
   npm install
   ```

2. **Start development server:**
   ```bash
   npm run dev
   ```

3. **Build for production:**
   ```bash
   npm run build
   ```

## Deployment to ESP32

1. **Build the frontend:**
   ```bash
   npm run build
   ```

2. **Upload to ESP32:**
   ```bash
   # From project root directory
   python upload_data.py
   ```

3. **Flash ESP32:**
   ```bash
   pio run --target upload
   ```

## Project Structure

```
data/
├── index.html          # Main HTML template
├── index.tsx           # React TypeScript application
├── index.css           # Styles with dark theme and animations
├── package.json        # Dependencies (React, TypeScript, Vite)
├── tsconfig.json       # TypeScript configuration
├── vite.config.ts      # Vite build configuration
├── dulux.json          # Dulux color database
└── README.md           # This file
```

## Key Technologies

- **React 19**: Latest React with concurrent features
- **TypeScript**: Type-safe development
- **Vite**: Fast build tool and dev server
- **CSS Custom Properties**: Modern styling approach
- **Font Awesome**: Icon library
- **ESP32**: Microcontroller with WiFi
- **TCS3430**: High-accuracy color sensor

## Recent Updates

- ✅ Removed Gemini AI dependency for faster, offline operation
- ✅ Integrated server-side Dulux color database
- ✅ Enhanced dark theme with purple accents and gradients
- ✅ Added Font Awesome icons throughout interface
- ✅ Implemented smooth animations and transitions
- ✅ Optimized live polling for 500ms updates
- ✅ Simplified capture process for instant feedback
- ✅ Added comprehensive error handling and fallbacks

## License

This project is part of the ESP32 Color Sensor system.
