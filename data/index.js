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

// Separate tracking for fast data and color names
let fastColorData = null;
let colorNameData = { colorName: 'Loading...', timestamp: 0 };
let lastFastUpdate = 0;
let lastColorNameUpdate = 0;

// Battery monitoring variables
let batteryData = { voltage: 0, percentage: 0, status: 'unknown' };
let lastBatteryUpdate = 0;

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
function updateLiveColorDisplay(fastData, colorNameInfo) {
    const container = document.getElementById('liveColorDisplay');
    if (!container) {
        console.error('liveColorDisplay container not found!');
        return;
    }

    // Validate fast color data
    if (!fastData || typeof fastData.r === 'undefined' || typeof fastData.g === 'undefined' || typeof fastData.b === 'undefined') {
        console.error('Invalid fast color data received:', fastData);
        container.innerHTML = `
            <div class="placeholder-container">
                <i class="fas fa-exclamation-triangle" style="color: #ef4444;"></i>
                <p class="placeholder-text">Invalid Data</p>
                <p class="placeholder-text" style="font-size: 0.9rem;">Sensor data incomplete</p>
            </div>
        `;
        return;
    }

    // Color stabilization - only update if color changed significantly
    const rgbDiff = Math.abs(fastData.r - lastR) + Math.abs(fastData.g - lastG) + Math.abs(fastData.b - lastB);
    const colorNameChanged = colorNameInfo && colorNameInfo.colorName !== lastColorName;
    
    // Only update display if RGB changed significantly (threshold of 3 for faster updates) or color name changed
    if (!colorNameChanged && rgbDiff < 3) {
        return; // Skip to avoid excessive DOM updates
    }

    lastR = fastData.r;
    lastG = fastData.g;
    lastB = fastData.b;
    
    if (colorNameInfo && colorNameInfo.colorName) {
        lastColorName = colorNameInfo.colorName;
    }

    const hex = rgbToHex(fastData.r, fastData.g, fastData.b);
    const displayName = colorNameInfo ? colorNameInfo.colorName : lastColorName || 'Live Sensor Feed';
    
    // Show if color name is updating
    const colorNameStatus = colorNameInfo && colorNameInfo.lookupInProgress ? ' (updating...)' : '';

    container.innerHTML = `
        <div class="color-display">
            <div class="color-swatch live-swatch" style="background-color: ${hex};"></div>
            <div class="color-details">
                <p class="color-name">${displayName}${colorNameStatus}</p>
                <p class="color-values">RGB: ${fastData.r}, ${fastData.g}, ${fastData.b}</p>
                <p class="color-values">HEX: ${hex}</p>
                <p class="color-values" style="font-size: 0.8rem; opacity: 0.7;">
                    Sensor: ${new Date(fastData.timestamp).toLocaleTimeString()}
                    ${colorNameInfo && colorNameInfo.colorNameTimestamp ? 
                      ` | Color: ${new Date(colorNameInfo.colorNameTimestamp).toLocaleTimeString()}` : ''}
                </p>
            </div>
        </div>
    `;

    liveColor = {
        id: 0,
        name: displayName,
        rgb: { r: fastData.r, g: fastData.g, b: fastData.b },
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

async function fetchFastColor() {
    try {
        const response = await fetch('/api/color-fast');

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const data = await response.json();
        fastColorData = data;
        lastFastUpdate = Date.now();
        
        // Update battery data if available in fast color response
        if (data.batteryVoltage !== undefined) {
            batteryData.voltage = data.batteryVoltage;
            // Calculate percentage based on voltage (rough estimate)
            if (data.batteryVoltage > 4.0) {
                batteryData.percentage = 100;
                batteryData.status = 'excellent';
            } else if (data.batteryVoltage > 3.7) {
                batteryData.percentage = Math.round((data.batteryVoltage - 3.0) / 1.2 * 100);
                batteryData.status = 'good';
            } else if (data.batteryVoltage > 3.4) {
                batteryData.percentage = Math.round((data.batteryVoltage - 3.0) / 1.2 * 100);
                batteryData.status = 'low';
            } else {
                batteryData.percentage = 0;
                batteryData.status = 'critical';
            }
            updateBatteryDisplay();
        }
        
        // Update display with current fast data and last known color name
        updateLiveColorDisplay(fastColorData, colorNameData);

    } catch (error) {
        console.error('Fast color API error:', error);
        showConnectionError('Fast Color', error.message);
    }
}

async function fetchColorName() {
    try {
        const response = await fetch('/api/color-name');

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const data = await response.json();
        colorNameData = data;
        lastColorNameUpdate = Date.now();
        
        // Update display with current color name and last known fast data
        if (fastColorData) {
            updateLiveColorDisplay(fastColorData, colorNameData);
        }

    } catch (error) {
        console.error('Color name API error:', error);
        // Don't show error for color names - just continue with last known name
    }
}

// Battery monitoring functions
async function fetchBatteryStatus() {
    try {
        const response = await fetch('/api/battery');

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const data = await response.json();
        batteryData = {
            voltage: data.batteryVoltage,
            percentage: data.percentage,
            status: data.status
        };
        lastBatteryUpdate = Date.now();
        
        updateBatteryDisplay();

    } catch (error) {
        console.error('Battery API error:', error);
        // Continue with last known battery data
    }
}

function updateBatteryDisplay() {
    const voltageElement = document.getElementById('batteryVoltage');
    const iconElement = document.getElementById('batteryIcon');
    
    if (!voltageElement || !iconElement) {
        console.error('Battery display elements not found');
        return;
    }

    // Update voltage display
    voltageElement.textContent = `${batteryData.voltage.toFixed(2)}V`;
    
    // Update icon based on status
    iconElement.className = `fas fa-battery-full ${batteryData.status}`;
    
    // Update icon based on percentage
    if (batteryData.percentage > 75) {
        iconElement.className = `fas fa-battery-full ${batteryData.status}`;
    } else if (batteryData.percentage > 50) {
        iconElement.className = `fas fa-battery-three-quarters ${batteryData.status}`;
    } else if (batteryData.percentage > 25) {
        iconElement.className = `fas fa-battery-half ${batteryData.status}`;
    } else if (batteryData.percentage > 10) {
        iconElement.className = `fas fa-battery-quarter ${batteryData.status}`;
    } else {
        iconElement.className = `fas fa-battery-empty ${batteryData.status}`;
    }
}

// Legacy function for backwards compatibility - combines both fast and color name data
async function fetchLiveColor() {
    try {
        const response = await fetch('/api/color');

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const data = await response.json();
        // Convert to new format
        fastColorData = {
            r: data.r, g: data.g, b: data.b,
            x: data.x, y: data.y, z: data.z,
            ir1: data.ir1, ir2: data.ir2,
            timestamp: data.timestamp
        };
        colorNameData = {
            colorName: data.colorName,
            colorNameTimestamp: data.timestamp
        };
        
        updateLiveColorDisplay(fastColorData, colorNameData);

    } catch (error) {
        console.error('API error:', error);
        showConnectionError('Connection', error.message);
    }
}

function showConnectionError(type, message) {
    const container = document.getElementById('liveColorDisplay');
    container.innerHTML = `
        <div class="placeholder-container">
            <i class="fas fa-exclamation-triangle" style="color: #ef4444;"></i>
            <p class="placeholder-text">${type} Error</p>
            <p class="placeholder-text" style="font-size: 0.9rem;">${message}</p>
            <p class="placeholder-text" style="font-size: 0.8rem;">Retrying...</p>
        </div>
    `;

    // Also show error in page title for debugging
    document.title = `Color Matcher - ${type} Error: ${message}`;
    setTimeout(() => {
        document.title = 'Color Matcher - Live View';
    }, 2000);
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
    
    // Calibration Mode
    const calibrationMode = currentSettings.calibrationMode || 'custom';
    document.getElementById('calibrationMode').value = calibrationMode;
    
    // Show/hide quadratic calibration controls based on mode
    const quadraticSections = document.querySelectorAll('.setting-group h3');
    let quadraticSection = null;
    for (let section of quadraticSections) {
        if (section.textContent.includes('Quadratic Calibration')) {
            quadraticSection = section.parentElement;
            break;
        }
    }
    
    if (quadraticSection) {
        if (calibrationMode === 'dfrobot') {
            quadraticSection.style.opacity = '0.5';
            quadraticSection.style.pointerEvents = 'none';
        } else {
            quadraticSection.style.opacity = '1';
            quadraticSection.style.pointerEvents = 'auto';
        }
    }
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
    // This function is replaced by resetToDFRobot and resetToCustom
    console.warn('resetToDefaults called - this should use specific reset functions');
}

// Reset to DFRobot library defaults
async function resetToDFRobot() {
    if (confirm('Reset to DFRobot library defaults? This uses the standard sensor matrix conversion.')) {
        try {
            const response = await fetch('/api/reset-to-dfrobot', { method: 'POST' });
            const result = await response.json();
            
            if (result.status === 'success') {
                console.log('Reset to DFRobot defaults successful');
                await loadCurrentSettings();
                await loadCalibrationSettings();
                setTimeout(() => location.reload(), 1000);
            } else {
                alert('Failed to reset: ' + (result.error || 'Unknown error'));
            }
        } catch (error) {
            console.error('Reset to DFRobot failed:', error);
            alert('Failed to reset to DFRobot defaults');
        }
    }
}

// Reset to custom quadratic calibration defaults
async function resetToCustom() {
    if (confirm('Reset to custom quadratic calibration defaults? This uses advanced tunable equations.')) {
        try {
            const response = await fetch('/api/reset-to-custom', { method: 'POST' });
            const result = await response.json();
            
            if (result.status === 'success') {
                console.log('Reset to custom defaults successful');
                await loadCurrentSettings();
                await loadCalibrationSettings();
                setTimeout(() => location.reload(), 1000);
            } else {
                alert('Failed to reset: ' + (result.error || 'Unknown error'));
            }
        } catch (error) {
            console.error('Reset to custom failed:', error);
            alert('Failed to reset to custom defaults');
        }
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
    
    // Calibration control event listeners
    document.getElementById('applyCalibration').addEventListener('click', updateCalibrationCoefficients);
    document.getElementById('resetCalibration').addEventListener('click', resetCalibrationToDefaults);
    document.getElementById('tuneVividWhite').addEventListener('click', tuneForVividWhite);
    
    // Action buttons
    document.getElementById('loadSettings').addEventListener('click', loadCurrentSettings);
    document.getElementById('saveSettings').addEventListener('click', applyAllSettings);
    document.getElementById('resetToDFRobot').addEventListener('click', resetToDFRobot);
    document.getElementById('resetToCustom').addEventListener('click', resetToCustom);
    
    // Calibration mode selector
    document.getElementById('calibrationMode').addEventListener('change', async function() {
        const mode = this.value;
        try {
            const response = await fetch(`/api/set-calibration-mode?mode=${mode}`);
            const result = await response.json();
            
            if (result.status === 'success') {
                console.log(`Calibration mode set to: ${mode}`);
                // Update UI to show/hide calibration controls based on mode
                const quadraticSections = document.querySelectorAll('.setting-group h3');
                let quadraticSection = null;
                for (let section of quadraticSections) {
                    if (section.textContent.includes('Quadratic Calibration')) {
                        quadraticSection = section.parentElement;
                        break;
                    }
                }
                if (quadraticSection) {
                    if (mode === 'dfrobot') {
                        // Hide quadratic calibration controls when using DFRobot mode
                        quadraticSection.style.opacity = '0.5';
                        quadraticSection.style.pointerEvents = 'none';
                    } else {
                        // Show quadratic calibration controls for custom mode
                        quadraticSection.style.opacity = '1';
                        quadraticSection.style.pointerEvents = 'auto';
                    }
                }
            } else {
                alert('Failed to set calibration mode: ' + (result.error || 'Unknown error'));
            }
        } catch (error) {
            console.error('Failed to set calibration mode:', error);
            alert('Failed to set calibration mode');
        }
    });
}

// Calibration Settings Management
let calibrationSettings = {
    redA: 5.756615248518086e-06,
    redB: -0.10824971353127427,
    redC: 663.2283515839658,
    greenA: 7.700364703908128e-06,
    greenB: -0.14873455804115546,
    greenC: 855.288778468652,
    blueA: -2.7588632792769936e-06,
    blueB: 0.04959423885676833,
    blueC: 35.55576869603341
};

// Load current calibration settings from ESP32
async function loadCalibrationSettings() {
    try {
        const response = await fetch('/api/calibration');
        if (response.ok) {
            calibrationSettings = await response.json();
            updateCalibrationUI();
            console.log('Calibration settings loaded:', calibrationSettings);
        } else {
            console.error('Failed to load calibration settings:', response.status);
        }
    } catch (error) {
        console.error('Error loading calibration settings:', error);
    }
}

// Update the calibration UI with current values
function updateCalibrationUI() {
    // Update calibration mode if available
    if (calibrationSettings.calibrationMode) {
        document.getElementById('calibrationMode').value = calibrationSettings.calibrationMode;
        
        // Show/hide quadratic calibration controls
        const quadraticSections = document.querySelectorAll('.setting-group h3');
        let quadraticSection = null;
        for (let section of quadraticSections) {
            if (section.textContent.includes('Quadratic Calibration')) {
                quadraticSection = section.parentElement;
                break;
            }
        }
        
        if (quadraticSection) {
            if (calibrationSettings.calibrationMode === 'dfrobot') {
                quadraticSection.style.opacity = '0.5';
                quadraticSection.style.pointerEvents = 'none';
            } else {
                quadraticSection.style.opacity = '1';
                quadraticSection.style.pointerEvents = 'auto';
            }
        }
    }
    
    // Red channel
    document.getElementById('redA').value = (calibrationSettings.redA * 1e6).toFixed(2);
    document.getElementById('redB').value = calibrationSettings.redB.toFixed(6);
    document.getElementById('redC').value = calibrationSettings.redC.toFixed(1);
    
    // Green channel
    document.getElementById('greenA').value = (calibrationSettings.greenA * 1e6).toFixed(2);
    document.getElementById('greenB').value = calibrationSettings.greenB.toFixed(6);
    document.getElementById('greenC').value = calibrationSettings.greenC.toFixed(1);
    
    // Blue channel
    document.getElementById('blueA').value = (calibrationSettings.blueA * 1e6).toFixed(2);
    document.getElementById('blueB').value = calibrationSettings.blueB.toFixed(6);
    document.getElementById('blueC').value = calibrationSettings.blueC.toFixed(1);
}

// Send calibration update to ESP32
async function updateCalibrationCoefficients() {
    try {
        const coefficients = {
            redA: parseFloat(document.getElementById('redA').value) * 1e-6,
            redB: parseFloat(document.getElementById('redB').value),
            redC: parseFloat(document.getElementById('redC').value),
            greenA: parseFloat(document.getElementById('greenA').value) * 1e-6,
            greenB: parseFloat(document.getElementById('greenB').value),
            greenC: parseFloat(document.getElementById('greenC').value),
            blueA: parseFloat(document.getElementById('blueA').value) * 1e-6,
            blueB: parseFloat(document.getElementById('blueB').value),
            blueC: parseFloat(document.getElementById('blueC').value)
        };

        // Use query parameters for ESP32 compatibility
        const params = new URLSearchParams();
        for (const [key, value] of Object.entries(coefficients)) {
            params.append(key, value);
        }

        const response = await fetch(`/api/calibration?${params.toString()}`, {
            method: 'POST'
        });
        
        if (response.ok) {
            const result = await response.json();
            console.log('Calibration updated:', result);
            calibrationSettings = coefficients;
            showNotification('Calibration coefficients updated successfully!');
        } else {
            console.error('Failed to update calibration:', response.status);
            showNotification('Failed to update calibration coefficients');
        }
    } catch (error) {
        console.error('Error updating calibration:', error);
        showNotification('Error updating calibration coefficients');
    }
}

// Quick tuning for specific target colors
async function tuneForVividWhite() {
    try {
        const response = await fetch('/api/tune-vivid-white', { method: 'POST' });
        if (response.ok) {
            const result = await response.json();
            console.log('Tuned for Vivid White:', result);
            calibrationSettings = result.calibration;
            updateCalibrationUI();
            showNotification('Tuned for Vivid White (247,248,244)');
        } else {
            console.error('Failed to tune for Vivid White:', response.status);
            showNotification('Failed to tune for Vivid White');
        }
    } catch (error) {
        console.error('Error tuning for Vivid White:', error);
        showNotification('Error tuning for Vivid White');
    }
}

// Reset calibration to defaults
function resetCalibrationToDefaults() {
    if (confirm('Reset calibration coefficients to factory defaults?')) {
        calibrationSettings = {
            redA: 5.756615248518086e-06,
            redB: -0.10824971353127427,
            redC: 663.2283515839658,
            greenA: 7.700364703908128e-06,
            greenB: -0.14873455804115546,
            greenC: 855.288778468652,
            blueA: -2.7588632792769936e-06,
            blueB: 0.04959423885676833,
            blueC: 35.55576869603341
        };
        updateCalibrationUI();
        updateCalibrationCoefficients();
    }
}

// Initialize application
function init() {
    // Load saved colors
    loadSavedColors();
    
    // Set up capture button
    const captureBtn = document.getElementById('captureBtn');
    captureBtn.addEventListener('click', captureColor);
    captureBtn.disabled = true; // Initially disabled until first color is fetched
    
    // Start fetching data immediately
    fetchFastColor();
    fetchColorName();
    fetchBatteryStatus();
    
    // Set up fast polling for sensor data (75ms for very smooth real-time updates)
    setInterval(fetchFastColor, 75);
    
    // Set up slower polling for color names (2 seconds - matches ESP32 internal lookup interval)
    setInterval(fetchColorName, 2000);
    
    // Set up battery monitoring (update every 10 seconds)
    setInterval(fetchBatteryStatus, 10000);

    // Load current settings
    loadCurrentSettings();

    // Load calibration settings
    loadCalibrationSettings();

    // Initialize settings listeners
    initializeSettingsListeners();
}

// Start the application when DOM is loaded
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
} else {
    init();
}
