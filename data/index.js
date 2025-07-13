// Utility function to convert RGB to HEX
function rgbToHex(r, g, b) {
    return "#" + [r, g, b].map(x => {
        const hex = x.toString(16);
        return hex.length === 1 ? '0' + hex : hex;
    }).join('');
}

// Global variables
let liveColor = null;
let scannedColor = null;
let savedColors = [];

// Color stabilization variables
let lastColorName = '';
let lastR = 0, lastG = 0, lastB = 0;

// Load saved colors from localStorage
function loadSavedColors() {
    try {
        const stored = localStorage.getItem('savedColors');
        if (stored) {
            savedColors = JSON.parse(stored);
            updateSavedColorsDisplay();
        }
    } catch (e) {
        console.error("Failed to load colors from localStorage", e);
    }
}

// Save colors to localStorage
function saveSavedColors() {
    try {
        localStorage.setItem('savedColors', JSON.stringify(savedColors));
    } catch (e) {
        console.error("Failed to save colors to localStorage", e);
    }
}

// Update the live color display
function updateLiveColorDisplay(colorData) {

    const container = document.getElementById('liveColorDisplay');
    if (!container) {
        console.error('liveColorDisplay container not found!');
        return;
    }

    // Validate color data
    if (!colorData || typeof colorData.r === 'undefined' || typeof colorData.g === 'undefined' || typeof colorData.b === 'undefined') {
        console.error('Invalid color data received:', colorData);
        container.innerHTML = `
            <div class="placeholder-container">
                <i class="fas fa-exclamation-triangle" style="color: #ef4444;"></i>
                <p class="placeholder-text">Invalid Data</p>
                <p class="placeholder-text" style="font-size: 0.9rem;">Sensor data incomplete</p>
            </div>
        `;
        return;
    }

    // Color stabilization - only update if color name has changed or significant RGB change
    const rgbDiff = Math.abs(colorData.r - lastR) + Math.abs(colorData.g - lastG) + Math.abs(colorData.b - lastB);
    const colorNameChanged = colorData.colorName !== lastColorName;
    
    // Only update if color name changed or RGB changed significantly (threshold of 10)
    if (!colorNameChanged && rgbDiff < 10) {
        return; // Skip silently to avoid console jitter
    }

    lastColorName = colorData.colorName;
    lastR = colorData.r;
    lastG = colorData.g;
    lastB = colorData.b;

    const hex = rgbToHex(colorData.r, colorData.g, colorData.b);

    container.innerHTML = `
        <div class="color-display">
            <div class="color-swatch live-swatch" style="background-color: ${hex};"></div>
            <div class="color-details">
                <p class="color-name">${colorData.colorName || 'Live Sensor Feed'}</p>
                <p class="color-values">RGB: ${colorData.r}, ${colorData.g}, ${colorData.b}</p>
                <p class="color-values">HEX: ${hex}</p>
                <p class="color-values" style="font-size: 0.8rem; opacity: 0.7;">Updated: ${new Date().toLocaleTimeString()}</p>
            </div>
        </div>
    `;

    liveColor = {
        id: 0,
        name: colorData.colorName || 'Live Sensor Feed',
        rgb: { r: colorData.r, g: colorData.g, b: colorData.b },
        hex: hex
    };

    // Enable capture button
    const captureBtn = document.getElementById('captureBtn');
    if (captureBtn) {
        captureBtn.disabled = false;
    } else {
        console.error('Capture button not found!');
    }
}

// Update the scanned color display
function updateScannedColorDisplay(color) {
    const container = document.getElementById('scannedColorDisplay');
    
    if (color) {
        container.innerHTML = `
            <div class="color-display">
                <div class="color-swatch" style="background-color: ${color.hex};"></div>
                <div class="color-details">
                    <p class="color-name">${color.name}</p>
                    <p class="color-values">RGB: ${color.rgb.r}, ${color.rgb.g}, ${color.rgb.b}</p>
                    <p class="color-values">HEX: ${color.hex}</p>
                    <button class="save-button" onclick="saveColor()">
                        <i class="fas fa-save"></i> Save Sample
                    </button>
                </div>
            </div>
        `;
    } else {
        container.innerHTML = `
            <div class="placeholder-container">
                <i class="fas fa-palette"></i>
                <p class="placeholder-text">Capture a color to see it here</p>
            </div>
        `;
    }
}

// Update saved colors display
function updateSavedColorsDisplay() {
    const section = document.getElementById('savedColorsSection');
    const grid = document.getElementById('savedColorsGrid');
    
    if (savedColors.length > 0) {
        section.style.display = 'block';
        grid.innerHTML = savedColors.map(color => `
            <div class="saved-color-card">
                <div class="saved-color-swatch" style="background-color: ${color.hex};"></div>
                <div class="saved-color-info">
                    <p class="saved-color-name">${color.name}</p>
                    <p class="saved-color-values">${color.hex}</p>
                </div>
                <button class="delete-button" onclick="deleteColor(${color.id})">
                    <i class="fas fa-trash"></i>
                </button>
            </div>
        `).join('');
    } else {
        section.style.display = 'none';
    }
}

// Fetch live color data from ESP32
async function fetchLiveColor() {
    try {
        const response = await fetch('/api/color');

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const data = await response.json();
        updateLiveColorDisplay(data);

    } catch (error) {
        console.error('API error:', error);
        // Show detailed error in live display
        const container = document.getElementById('liveColorDisplay');
        container.innerHTML = `
            <div class="placeholder-container">
                <i class="fas fa-exclamation-triangle" style="color: #ef4444;"></i>
                <p class="placeholder-text">Connection Error</p>
                <p class="placeholder-text" style="font-size: 0.9rem;">${error.message}</p>
                <p class="placeholder-text" style="font-size: 0.8rem;">Retrying in 500ms...</p>
            </div>
        `;

        // Also show error in page title for debugging
        document.title = `Color Matcher - Error: ${error.message}`;
        setTimeout(() => {
            document.title = 'Color Matcher - Live View';
        }, 2000);
    }
}

// Capture current live color
function captureColor() {
    if (!liveColor) return;
    
    scannedColor = {
        ...liveColor,
        id: Date.now()
    };
    
    updateScannedColorDisplay(scannedColor);
}

// Save scanned color to saved colors
function saveColor() {
    if (!scannedColor) return;
    
    // Check if color already exists
    const exists = savedColors.some(c => c.id === scannedColor.id);
    if (exists) return;
    
    // Add to beginning of array
    savedColors.unshift(scannedColor);
    
    // Limit to 20 saved colors
    if (savedColors.length > 20) {
        savedColors = savedColors.slice(0, 20);
    }
    
    saveSavedColors();
    updateSavedColorsDisplay();
    
    // Clear scanned color
    scannedColor = null;
    updateScannedColorDisplay(null);
    
    // Show success feedback
    showNotification('Color saved successfully!');
}

// Delete a saved color
function deleteColor(id) {
    savedColors = savedColors.filter(color => color.id !== id);
    saveSavedColors();
    updateSavedColorsDisplay();
}

// Show notification
function showNotification(message) {
    // Simple alert for now - could be enhanced with a toast notification
    alert(message);
}

// Initialize the application
function init() {
    // Load saved colors
    loadSavedColors();
    
    // Set up capture button
    const captureBtn = document.getElementById('captureBtn');
    captureBtn.addEventListener('click', captureColor);
    captureBtn.disabled = true; // Initially disabled until first color is fetched
    
    // Start fetching live color data
    fetchLiveColor();
    
    // Set up interval to fetch live color every 250ms for smoother experience
    setInterval(fetchLiveColor, 150); // Maximum responsiveness for real-time color detection
}

// Start the application when DOM is loaded
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
} else {
    init();
}
