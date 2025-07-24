// TCS3430 Calibration System JavaScript
// Clean, focused implementation for the new calibration system

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
let calibrationData = null;
let lastR = 0, lastG = 0, lastB = 0;

// Battery monitoring variables
let batteryData = { voltage: 0, percentage: 0, status: 'unknown' };

// Calibration status tracking
let calibrationStatus = {
    isCalibrated: false,
    blackReference: null,
    whiteReference: null,
    vividWhiteCalibrated: false
};

// Update calibration status display
function updateCalibrationStatus() {
    fetch('/api/calibration-data')
        .then(response => response.json())
        .then(data => {
            calibrationStatus = data;
            
            // Update main status indicator
            const indicator = document.getElementById('calibrationIndicator');
            const statusText = document.getElementById('calibrationStatusText');
            const mainCalibrationStatus = document.getElementById('calibrationStatus');
            
            if (data.isCalibrated) {
                indicator.className = 'fas fa-circle status-good';
                statusText.textContent = 'Calibration Complete';
                mainCalibrationStatus.textContent = 'Calibrated ✓';
            } else {
                indicator.className = 'fas fa-circle status-warning';
                statusText.textContent = 'Calibration Required';
                mainCalibrationStatus.textContent = 'Not Calibrated';
            }
            
            // Update individual step status using explicit flags
            // Use new explicit completion flags if available, otherwise fall back to data detection
            const blackCalibrated = data.blackReferenceComplete || 
                (data.blackReference && (data.blackReference.X > 0 || data.blackReference.Y > 0 || data.blackReference.Z > 0));
            const whiteCalibrated = data.whiteReferenceComplete ||
                (data.whiteReference && (data.whiteReference.X > 100 || data.whiteReference.Y > 100 || data.whiteReference.Z > 100));

            updateStepStatus('blackStatus', blackCalibrated);
            updateStepStatus('whiteStatus', whiteCalibrated);
            updateStepStatus('vividWhiteStatus', data.isCalibrated);
            
            // Update progress indicator
            if (blackCalibrated && !whiteCalibrated) {
                statusText.textContent = 'Black Reference Complete - White Reference Needed';
                mainCalibrationStatus.textContent = 'Partially Calibrated (1/2)';
            } else if (blackCalibrated && whiteCalibrated && !data.isCalibrated) {
                statusText.textContent = 'References Complete - Vivid White Calibration Needed';
                mainCalibrationStatus.textContent = 'Partially Calibrated (2/3)';
            }
        })
        .catch(error => {
            console.error('Failed to get calibration status:', error);
            document.getElementById('calibrationStatusText').textContent = 'Status Check Failed';
        });
}

function updateStepStatus(elementId, isComplete) {
    const element = document.getElementById(elementId);
    if (element) {
        element.textContent = isComplete ? 'Completed ✓' : 'Not calibrated';
        element.className = isComplete ? 'step-status completed' : 'step-status pending';
    }
}

// Calibration button handlers
function setupCalibrationHandlers() {
    // Black reference calibration
    document.getElementById('calibrateBlackBtn').addEventListener('click', async () => {
        const btn = document.getElementById('calibrateBlackBtn');
        btn.disabled = true;
        btn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Calibrating...';
        
        try {
            const response = await fetch('/api/tune-black', { method: 'POST' });
            const data = await response.json();
            
            if (data.status === 'success') {
                showNotification('Black reference calibrated successfully!', 'success');
                updateCalibrationStatus();
            } else {
                showNotification('Black calibration failed: ' + (data.message || data.error || 'Unknown error'), 'error');
            }
        } catch (error) {
            showNotification('Black calibration error: ' + error.message, 'error');
        } finally {
            btn.disabled = false;
            btn.innerHTML = '<i class="fas fa-square" style="color: #000;"></i> Calibrate Black Reference';
        }
    });

    // White reference calibration
    document.getElementById('calibrateWhiteBtn').addEventListener('click', async () => {
        const btn = document.getElementById('calibrateWhiteBtn');
        btn.disabled = true;
        btn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Calibrating...';
        
        try {
            const response = await fetch('/api/calibrate-white', { method: 'POST' });
            const data = await response.json();
            
            if (data.status === 'success') {
                showNotification('White reference calibrated successfully!', 'success');
                updateCalibrationStatus();
            } else {
                showNotification('White calibration failed: ' + (data.message || data.error || 'Unknown error'), 'error');
            }
        } catch (error) {
            showNotification('White calibration error: ' + error.message, 'error');
        } finally {
            btn.disabled = false;
            btn.innerHTML = '<i class="fas fa-square" style="color: #fff; border: 1px solid #ccc;"></i> Calibrate White Reference';
        }
    });

    // Vivid white calibration
    document.getElementById('calibrateVividWhiteBtn').addEventListener('click', async () => {
        const btn = document.getElementById('calibrateVividWhiteBtn');
        btn.disabled = true;
        btn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Calibrating...';
        
        try {
            const response = await fetch('/api/calibrate-vivid-white', { method: 'POST' });
            const data = await response.json();
            
            if (data.status === 'success') {
                showNotification('Vivid white calibrated successfully!', 'success');
                updateCalibrationStatus();
            } else {
                showNotification('Vivid white calibration failed: ' + (data.message || data.error || 'Unknown error'), 'error');
            }
        } catch (error) {
            showNotification('Vivid white calibration error: ' + error.message, 'error');
        } finally {
            btn.disabled = false;
            btn.innerHTML = '<i class="fas fa-bullseye"></i> Calibrate Vivid White Target';
        }
    });
}

// Fetch live color data from ESP32
async function fetchFastColor() {
    try {
        const response = await fetch('/api/color');
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}`);
        }
        
        const data = await response.json();
        updateLiveColorDisplay(data);
        updateSensorStatus('Connected', true);
        
    } catch (error) {
        console.error('Failed to fetch color data:', error);
        updateSensorStatus('Connection Error', false);
        
        // Show error in live display
        const container = document.getElementById('liveColorDisplay');
        if (container) {
            container.innerHTML = `
                <div class="placeholder-container">
                    <i class="fas fa-exclamation-triangle" style="color: #ef4444;"></i>
                    <p class="placeholder-text">Connection Error</p>
                    <p class="placeholder-text" style="font-size: 0.9rem;">Failed to fetch sensor data</p>
                </div>
            `;
        }
    }
}

// Update sensor status display
function updateSensorStatus(status, isConnected) {
    const statusText = document.getElementById('statusText');
    if (statusText) {
        statusText.textContent = status;
        statusText.style.color = isConnected ? '#10b981' : '#ef4444';
    }
}

// Update the live color display
function updateLiveColorDisplay(fastData) {
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
    
    // Only update display if RGB changed significantly (threshold of 3 for faster updates)
    if (rgbDiff < 3) {
        return; // Skip to avoid excessive DOM updates
    }

    lastR = fastData.r;
    lastG = fastData.g;
    lastB = fastData.b;

    const hex = rgbToHex(fastData.r, fastData.g, fastData.b);

    container.innerHTML = `
        <div class="color-display">
            <div class="color-swatch live-swatch" style="background-color: ${hex};"></div>
            <div class="color-details">
                <p class="color-name">Live TCS3430 Reading</p>
                <p class="color-values">RGB: ${fastData.r}, ${fastData.g}, ${fastData.b}</p>
                <p class="color-values">HEX: ${hex}</p>
                <p class="color-values">Sensor: X=${fastData.x || 'N/A'}, Y=${fastData.y || 'N/A'}, Z=${fastData.z || 'N/A'}</p>
                <p class="color-values">IR: IR1=${fastData.ir1 || 'N/A'}, IR2=${fastData.ir2 || 'N/A'}</p>
                <p class="color-values" style="font-size: 0.8rem; opacity: 0.7;">
                    Updated: ${new Date().toLocaleTimeString()}
                </p>
            </div>
        </div>
    `;

    liveColor = {
        id: 0,
        name: 'Live TCS3430 Reading',
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

// Optimization and testing handlers
function setupOptimizationHandlers() {
    // Optimize accuracy button
    document.getElementById('optimizeAccuracyBtn').addEventListener('click', async () => {
        const btn = document.getElementById('optimizeAccuracyBtn');
        btn.disabled = true;
        btn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Optimizing...';

        try {
            const response = await fetch('/api/optimize-accuracy', { method: 'POST' });
            const data = await response.json();

            if (data.status === 'success') {
                showNotification('Accuracy optimizations applied successfully!', 'success');
                // Update sample count display if available
                if (data.improvements && data.improvements.sampleCount) {
                    document.getElementById('colorSamplesValue').textContent = data.improvements.sampleCount.new;
                    document.getElementById('colorSamples').value = data.improvements.sampleCount.new;
                }
            } else {
                showNotification('Optimization failed: ' + (data.message || data.error || 'Unknown error'), 'error');
            }
        } catch (error) {
            showNotification('Optimization error: ' + error.message, 'error');
        } finally {
            btn.disabled = false;
            btn.innerHTML = '<i class="fas fa-magic"></i> Optimize for Maximum Accuracy';
        }
    });

    // Test all improvements button
    document.getElementById('testAllImprovementsBtn').addEventListener('click', async () => {
        const btn = document.getElementById('testAllImprovementsBtn');
        btn.disabled = true;
        btn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Testing...';

        try {
            const response = await fetch('/api/test-all-improvements');
            const data = await response.json();

            if (data.status === 'success') {
                displayTestResults(data);
                showNotification('Comprehensive test completed!', 'success');
            } else {
                showNotification('Test failed: ' + (data.message || data.error || 'Unknown error'), 'error');
            }
        } catch (error) {
            showNotification('Test error: ' + error.message, 'error');
        } finally {
            btn.disabled = false;
            btn.innerHTML = '<i class="fas fa-clipboard-check"></i> Test All Improvements';
        }
    });
}

function displayTestResults(data) {
    const resultsDiv = document.getElementById('testResults');
    const scoreSpan = document.getElementById('overallScore');
    const gradeSpan = document.getElementById('overallGrade');
    const recommendationsDiv = document.getElementById('recommendations');

    scoreSpan.textContent = data.overallScore + '/100';
    gradeSpan.textContent = data.grade;
    gradeSpan.className = 'grade grade-' + data.grade.toLowerCase();

    // Display recommendations
    if (data.recommendations && data.recommendations.length > 0) {
        recommendationsDiv.innerHTML = '<h4>Recommendations:</h4><ul>' +
            data.recommendations.map(rec => `<li>${rec}</li>`).join('') + '</ul>';
    } else {
        recommendationsDiv.innerHTML = '<h4>All tests passed! ✓</h4>';
    }

    resultsDiv.style.display = 'block';
}

// Battery monitoring
async function fetchBatteryStatus() {
    try {
        const response = await fetch('/api/battery');
        if (response.ok) {
            const data = await response.json();
            // Validate the received data
            if (data && typeof data === 'object') {
                batteryData = data;
                updateBatteryDisplay();
            } else {
                console.warn('Invalid battery data received:', data);
                batteryData = null;
                updateBatteryDisplay();
            }
        } else {
            console.warn('Battery status request failed:', response.status);
            batteryData = null;
            updateBatteryDisplay();
        }
    } catch (error) {
        console.error('Failed to fetch battery status:', error);
        batteryData = null;
        updateBatteryDisplay();
    }
}

function updateBatteryDisplay() {
    const voltageElement = document.getElementById('batteryVoltage');
    const iconElement = document.getElementById('batteryIcon');

    // Check if batteryData exists and has required properties
    if (!batteryData || typeof batteryData.voltage === 'undefined') {
        if (voltageElement) {
            voltageElement.textContent = 'N/A';
        }
        if (iconElement) {
            iconElement.className = 'fas fa-battery-empty';
            iconElement.style.color = '#ef4444';
        }
        return;
    }

    if (voltageElement) {
        const voltage = parseFloat(batteryData.voltage);
        if (isNaN(voltage)) {
            voltageElement.textContent = 'N/A';
        } else {
            voltageElement.textContent = `${voltage.toFixed(2)}V`;
        }
    }

    if (iconElement) {
        const percentage = parseFloat(batteryData.percentage) || 0;

        // Update battery icon based on percentage
        if (percentage > 75) {
            iconElement.className = 'fas fa-battery-full';
            iconElement.style.color = '#10b981';
        } else if (percentage > 50) {
            iconElement.className = 'fas fa-battery-three-quarters';
            iconElement.style.color = '#f59e0b';
        } else if (percentage > 25) {
            iconElement.className = 'fas fa-battery-half';
            iconElement.style.color = '#f59e0b';
        } else {
            iconElement.className = 'fas fa-battery-quarter';
            iconElement.style.color = '#ef4444';
        }
    }
}

// Color capture functionality
async function captureColor() {
    if (!liveColor) {
        showNotification('No live color data available', 'error');
        return;
    }

    try {
        // Capture current color
        scannedColor = { ...liveColor };
        updateScannedColorDisplay(scannedColor);
        showNotification('Color captured successfully!', 'success');

    } catch (error) {
        console.error('Failed to capture color:', error);
        showNotification('Failed to capture color', 'error');
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
                    <p class="color-values" style="font-size: 0.8rem; opacity: 0.7;">
                        Captured: ${new Date().toLocaleTimeString()}
                    </p>
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

// Utility handlers
function setupUtilityHandlers() {
    // Get calibration data button
    document.getElementById('getCalibrationDataBtn').addEventListener('click', async () => {
        try {
            const response = await fetch('/api/calibration-data');
            const data = await response.json();

            const info = `Calibration Status: ${data.isCalibrated ? 'Complete' : 'Incomplete'}\n` +
                        `Black Reference: X=${data.blackReference?.X || 'N/A'}, Y=${data.blackReference?.Y || 'N/A'}, Z=${data.blackReference?.Z || 'N/A'}\n` +
                        `White Reference: X=${data.whiteReference?.X || 'N/A'}, Y=${data.whiteReference?.Y || 'N/A'}, Z=${data.whiteReference?.Z || 'N/A'}\n` +
                        `Target RGB: (${data.vividWhiteTarget?.R || 'N/A'}, ${data.vividWhiteTarget?.G || 'N/A'}, ${data.vividWhiteTarget?.B || 'N/A'})`;

            alert(info);
        } catch (error) {
            showNotification('Failed to get calibration data: ' + error.message, 'error');
        }
    });

    // Reset calibration button
    document.getElementById('resetCalibrationBtn').addEventListener('click', async () => {
        if (confirm('Are you sure you want to reset all calibration data? This will require recalibration.')) {
            try {
                const response = await fetch('/api/reset-calibration', { method: 'POST' });
                const data = await response.json();

                if (data.status === 'success') {
                    showNotification('Calibration reset successfully!', 'success');
                    updateCalibrationStatus();
                } else {
                    showNotification('Reset failed: ' + (data.message || data.error || 'Unknown error'), 'error');
                }
            } catch (error) {
                showNotification('Reset error: ' + error.message, 'error');
            }
        }
    });

    // Diagnose calibration button
    document.getElementById('diagnoseCalibrationBtn').addEventListener('click', async () => {
        try {
            const response = await fetch('/api/diagnose-calibration');
            const data = await response.json();

            const diagnosis = `Current RGB: (${data.currentRGB?.R || 'N/A'}, ${data.currentRGB?.G || 'N/A'}, ${data.currentRGB?.B || 'N/A'})\n` +
                            `Target RGB: (${data.targetRGB?.R || 'N/A'}, ${data.targetRGB?.G || 'N/A'}, ${data.targetRGB?.B || 'N/A'})\n` +
                            `Calibrated: ${data.isCalibrated ? 'Yes' : 'No'}\n` +
                            `Recommendation: ${data.recommendation || 'N/A'}`;

            alert(diagnosis);
        } catch (error) {
            showNotification('Diagnosis failed: ' + error.message, 'error');
        }
    });
}

// Setup sample count slider
function setupSampleCountSlider() {
    const slider = document.getElementById('colorSamples');
    const valueDisplay = document.getElementById('colorSamplesValue');

    if (slider && valueDisplay) {
        slider.addEventListener('input', function() {
            valueDisplay.textContent = this.value;
        });

        slider.addEventListener('change', async function() {
            try {
                const response = await fetch(`/api/set-color-samples?samples=${this.value}`, { method: 'POST' });
                const data = await response.json();

                if (data.status === 'success') {
                    showNotification(`Sample count updated to ${this.value}`, 'success');
                } else {
                    showNotification('Failed to update sample count', 'error');
                }
            } catch (error) {
                showNotification('Error updating sample count: ' + error.message, 'error');
            }
        });
    }
}

// Notification system
function showNotification(message, type = 'info') {
    // Create notification element
    const notification = document.createElement('div');
    notification.className = `notification notification-${type}`;
    notification.innerHTML = `
        <i class="fas fa-${type === 'success' ? 'check-circle' : type === 'error' ? 'exclamation-circle' : 'info-circle'}"></i>
        <span>${message}</span>
        <button class="notification-close" onclick="this.parentElement.remove()">×</button>
    `;

    // Add to page
    document.body.appendChild(notification);

    // Auto-remove after 5 seconds
    setTimeout(() => {
        if (notification.parentElement) {
            notification.remove();
        }
    }, 5000);
}

// Initialize TCS3430 Calibration System
function init() {
    console.log('Initializing TCS3430 Calibration System...');

    // Set up capture button
    const captureBtn = document.getElementById('captureBtn');
    if (captureBtn) {
        captureBtn.addEventListener('click', captureColor);
        captureBtn.disabled = true; // Initially disabled until first color is fetched
    }

    // Set up calibration handlers
    setupCalibrationHandlers();

    // Set up optimization handlers
    setupOptimizationHandlers();

    // Set up utility handlers
    setupUtilityHandlers();

    // Set up sample count slider
    setupSampleCountSlider();

    // Start fetching data immediately
    fetchFastColor();
    fetchBatteryStatus();

    // Set up fast polling for sensor data (75ms for very smooth real-time updates)
    setInterval(fetchFastColor, 75);

    // Set up battery monitoring (update every 10 seconds)
    setInterval(fetchBatteryStatus, 10000);

    // Update calibration status
    updateCalibrationStatus();

    console.log('TCS3430 Calibration System initialized successfully!');
}

// Start the application when DOM is loaded
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
} else {
    init();
}
