# ESP32 Optimized Upload Guide

This guide explains how to upload only the essential files to your ESP32, avoiding the "file system full" error.

## 🚀 Quick Start

### Windows
```bash
upload.bat
```

### Linux/Mac
```bash
chmod +x upload.sh
./upload.sh
```

### Manual
```bash
python upload_optimized.py
```

## 📋 What This Script Does

1. **Builds the frontend** - Compiles React TypeScript to optimized files
2. **Extracts essential files** - Only takes what the ESP32 needs
3. **Uploads efficiently** - Avoids uploading large node_modules

## 📁 Files Uploaded

### Frontend Files (Built)
- `index.html` - Main web page
- `index.css` - Compiled styles with dark theme
- `index.js` - Compiled React app (from index.tsx)

### Data Files
- `dulux.json` - Color database
- `metadata.json` - Project metadata

## 📊 Size Comparison

| Method | Size | Status |
|--------|------|--------|
| **Old (all files)** | ~50MB+ | ❌ Too large |
| **New (optimized)** | ~1-2MB | ✅ Fits perfectly |

## 🔧 Manual Build Process

If you prefer to build manually:

```bash
# 1. Build frontend
cd data
npm run build

# 2. Copy essential files to ESP32
# Copy from data/dist/ to ESP32:
# - index.html
# - assets/*.css → index.css  
# - assets/*.js → index.js
# - dulux.json
# - metadata.json
```

## 🐛 Troubleshooting

### "No space left" error
- Use the optimized script instead of uploading all files
- Check that you're not uploading node_modules

### Build fails
```bash
cd data
rm -rf node_modules
npm install
npm run build
```

### Upload fails
- Check ESP32 connection
- Verify COM port in platformio.ini
- Try resetting ESP32

## 📈 Benefits

- ✅ **Faster uploads** - Only essential files
- ✅ **No space issues** - Optimized size
- ✅ **Better performance** - Compiled/minified code
- ✅ **Automatic build** - No manual steps needed

## 🔄 Development Workflow

1. **Develop**: Edit files in `data/` directory
2. **Test**: Run `npm run dev` for local testing
3. **Deploy**: Run `upload_optimized.py` for ESP32
4. **Monitor**: Use `pio device monitor` to check logs

## 📝 Notes

- The script automatically backs up your original data directory
- Built files are optimized and minified for production
- Only essential files are uploaded to save space
- Original development files remain untouched
