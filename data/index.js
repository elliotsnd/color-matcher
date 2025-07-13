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

// Settings Management
let currentSettings = {}

// Load current settings from ESP32
async function loadCurrentSettings() {
    try {
        const response = await fetch('/api/settings');
        if (response.ok) {
            currentSettings = await response.json();
            updateSettingsUI();
            console.log('Settings loaded:', currentSettings);
        } else {
            console.error('Failed to load settings:', response.status);
        }
    } catch (error) {
        console.error('Error loading settings:', error);
    }
}

// Update the settings UI with current values
function updateSettingsUI() {
    // LED & Sensor Settings
    document.getElementById('ledBrightness').value = currentSettings.ledBrightness || 85;
    document.getElementById('ledBrightnessValue').textContent = currentSettings.ledBrightness || 85;
    
    document.getElementById('integrationTime').value = currentSettings.sensorIntegrationTime || 35;
    
    // Color Processing Settings
    document.getElementById('colorSamples').value = currentSettings.colorReadingSamples || 5;
    document.getElementById('colorSamplesValue').textContent = currentSettings.colorReadingSamples || 5;
    
    document.getElementById('sampleDelay').value = currentSettings.sensorSampleDelay || 2;
    document.getElementById('sampleDelayValue').textContent = currentSettings.sensorSampleDelay || 2;
    
    // IR Compensation Settings
    const ir1Value = Math.round((currentSettings.irCompensationFactor1 || 0.35) * 100);
    const ir2Value = Math.round((currentSettings.irCompensationFactor2 || 0.34) * 100);
    
    document.getElementById('irFactor1').value = ir1Value;
    document.getElementById('irFactor1Value').textContent = (ir1Value / 100).toFixed(2);
    
    document.getElementById('irFactor2').value = ir2Value;
    document.getElementById('irFactor2Value').textContent = (ir2Value / 100).toFixed(2);
    
    // Debug Settings
    document.getElementById('debugSensor').checked = currentSettings.debugSensorReadings || false;
    document.getElementById('debugColors').checked = currentSettings.debugColorMatching || false;
}

// Send settings update to ESP32
async function updateSetting(setting, value) {
    try {
        const body = { [setting]: value };
        const response = await fetch('/api/settings', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(body)
        });
        
        if (response.ok) {
            const result = await response.json();
            console.log('Setting updated:', setting, '=', value);
            currentSettings[setting] = value; // Update local copy
        } else {
            console.error('Failed to update setting:', response.status);
        }
    } catch (error) {
        console.error('Error updating setting:', error);
    }
}

// Quick update functions for real-time changes (using GET for simplicity)
async function updateLedBrightness(value) {
    try {
        const response = await fetch(`/api/set-led-brightness?value=${value}`);
        if (response.ok) {
            console.log('LED brightness updated to:', value);
        }
    } catch (error) {
        console.error('Error updating LED brightness:', error);
    }
}

async function updateIntegrationTime(value) {
    try {
        const response = await fetch(`/api/set-integration-time?value=${value}`);
        if (response.ok) {
            console.log('Integration time updated to:', value);
        }
    } catch (error) {
        console.error('Error updating integration time:', error);
    }
}

async function updateIRCompensation(ir1, ir2) {
    try {
        const response = await fetch(`/api/set-ir-factors?ir1=${ir1}&ir2=${ir2}`);
        if (response.ok) {
            console.log('IR compensation updated:', ir1, ir2);
        }
    } catch (error) {
        console.error('Error updating IR compensation:', error);
    }
}

async function updateColorSamples(value) {
    try {
        const response = await fetch(`/api/set-color-samples?value=${value}`);
        if (response.ok) {
            console.log('Color samples updated to:', value);
        }
    } catch (error) {
        console.error('Error updating color samples:', error);
    }
}

async function updateSampleDelay(value) {
    try {
        const response = await fetch(`/api/set-sample-delay?value=${value}`);
        if (response.ok) {
            console.log('Sample delay updated to:', value);
        }
    } catch (error) {
        console.error('Error updating sample delay:', error);
    }
}

async function updateDebugSettings(sensor, colors) {
    try {
        const response = await fetch(`/api/set-debug?sensor=${sensor}&colors=${colors}`);
        if (response.ok) {
            console.log('Debug settings updated:', sensor, colors);
        }
    } catch (error) {
        console.error('Error updating debug settings:', error);
    }
}

// Apply all current settings (mainly for batch updates of non-real-time settings)
async function applyAllSettings() {
    try {
        // Most settings are now updated in real-time, but we can still provide this for completeness
        console.log('All real-time settings are already applied!');
        alert('Settings are applied in real-time! All changes have been saved.');
    } catch (error) {
        console.error('Error applying settings:', error);
        alert('Error applying settings');
    }
}

// Reset settings to defaults
function resetToDefaults() {
    if (confirm('Reset all settings to defaults? This will reload the page.')) {
        // Reset to compile-time defaults
        currentSettings = {
            ledBrightness: 85,
            sensorIntegrationTime: 35, // 0x23
            colorReadingSamples: 5,
            sensorSampleDelay: 2,
            irCompensationFactor1: 0.35,
            irCompensationFactor2: 0.34,
            debugSensorReadings: true,
            debugColorMatching: true
        };
        updateSettingsUI();
        applyAllSettings().then(() => {
            setTimeout(() => location.reload(), 1000);
        });
    }
}

// Initialize settings event listeners
function initializeSettingsListeners() {
    // LED Brightness - real-time update
    const ledBrightness = document.getElementById('ledBrightness');
    const ledBrightnessValue = document.getElementById('ledBrightnessValue');
    
    ledBrightness.addEventListener('input', (e) => {
        const value = parseInt(e.target.value);
        ledBrightnessValue.textContent = value;
        currentSettings.ledBrightness = value;
        updateLedBrightness(value); // Real-time update
    });
    
    // Integration Time - immediate update
    const integrationTime = document.getElementById('integrationTime');
    integrationTime.addEventListener('change', (e) => {
        const value = parseInt(e.target.value);
        currentSettings.sensorIntegrationTime = value;
        updateIntegrationTime(value); // Real-time update
    });
    
    // Color Samples
    const colorSamples = document.getElementById('colorSamples');
    const colorSamplesValue = document.getElementById('colorSamplesValue');
    
    colorSamples.addEventListener('input', (e) => {
        const value = parseInt(e.target.value);
        colorSamplesValue.textContent = value;
        currentSettings.colorReadingSamples = value;
        updateColorSamples(value); // Real-time update
    });
    
    // Sample Delay
    const sampleDelay = document.getElementById('sampleDelay');
    const sampleDelayValue = document.getElementById('sampleDelayValue');
    
    sampleDelay.addEventListener('input', (e) => {
        const value = parseInt(e.target.value);
        sampleDelayValue.textContent = value;
        currentSettings.sensorSampleDelay = value;
        updateSampleDelay(value); // Real-time update
    });
    
    // IR Compensation factors - real-time update
    const irFactor1 = document.getElementById('irFactor1');
    const irFactor1Value = document.getElementById('irFactor1Value');
    const irFactor2 = document.getElementById('irFactor2');
    const irFactor2Value = document.getElementById('irFactor2Value');
    
    irFactor1.addEventListener('input', (e) => {
        const value = parseInt(e.target.value) / 100;
        irFactor1Value.textContent = value.toFixed(2);
        currentSettings.irCompensationFactor1 = value;
        const ir2 = currentSettings.irCompensationFactor2 || 0.34;
        updateIRCompensation(value, ir2); // Real-time update
    });
    
    irFactor2.addEventListener('input', (e) => {
        const value = parseInt(e.target.value) / 100;
        irFactor2Value.textContent = value.toFixed(2);
        currentSettings.irCompensationFactor2 = value;
        const ir1 = currentSettings.irCompensationFactor1 || 0.35;
        updateIRCompensation(ir1, value); // Real-time update
    });
    
    // Debug checkboxes - real-time update
    document.getElementById('debugSensor').addEventListener('change', (e) => {
        currentSettings.debugSensorReadings = e.target.checked;
        const colorsChecked = document.getElementById('debugColors').checked;
        updateDebugSettings(e.target.checked, colorsChecked);
    });
    
    document.getElementById('debugColors').addEventListener('change', (e) => {
        currentSettings.debugColorMatching = e.target.checked;
        const sensorChecked = document.getElementById('debugSensor').checked;
        updateDebugSettings(sensorChecked, e.target.checked);
    });
    
    // Action buttons
    document.getElementById('loadSettings').addEventListener('click', loadCurrentSettings);
    document.getElementById('saveSettings').addEventListener('click', applyAllSettings);
    document.getElementById('resetSettings').addEventListener('click', resetToDefaults);
}

// Initialize application
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

    // Load current settings
    loadCurrentSettings();

    // Initialize settings listeners
    initializeSettingsListeners();
}

// Start the application when DOM is loaded
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
} else {
    init();
}
