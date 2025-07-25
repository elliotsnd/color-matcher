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
    vividWhiteCalibrated: false,
    // Professional 4-point calibration
    is4PointCalibrated: false,
    blueReference: null,
    yellowReference: null,
    interpolationMethod: 'linear'
};

// Professional mode state
let professionalMode = false;

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
            
            // Update professional calibration status
            if (data.calibrationStatus) {
                calibrationStatus.is4PointCalibrated = data.calibrationStatus.is4PointCalibrated;
                calibrationStatus.interpolationMethod = data.calibrationStatus.interpolationMethod || 'linear';
            }

            // Update individual step status using explicit flags
            const blackCalibrated = data.blackReferenceComplete ||
                (data.calibrationStatus && data.calibrationStatus.blackComplete);
            const whiteCalibrated = data.whiteReferenceComplete ||
                (data.calibrationStatus && data.calibrationStatus.whiteComplete);
            const blueCalibrated = data.calibrationStatus && data.calibrationStatus.blueComplete;
            const yellowCalibrated = data.calibrationStatus && data.calibrationStatus.yellowComplete;

            updateStepStatus('blackStatus', blackCalibrated);
            updateStepStatus('whiteStatus', whiteCalibrated);
            updateStepStatus('blueStatus', blueCalibrated);
            updateStepStatus('yellowStatus', yellowCalibrated);
            updateStepStatus('vividWhiteStatus', data.isCalibrated);

            // Update progress indicator based on calibration mode
            if (professionalMode) {
                updateProfessionalProgress(blackCalibrated, whiteCalibrated, blueCalibrated, yellowCalibrated, data.isCalibrated);
            } else {
                updateStandardProgress(blackCalibrated, whiteCalibrated, data.isCalibrated);
            }

            // Update professional status display
            if (calibrationStatus.is4PointCalibrated) {
                statusText.textContent += ' (Professional 4-Point)';
                mainCalibrationStatus.textContent += ' - Tetrahedral Interpolation';
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

function updateStandardProgress(blackCalibrated, whiteCalibrated, isCalibrated) {
    const statusText = document.getElementById('calibrationStatusText');
    const mainCalibrationStatus = document.getElementById('calibrationStatus');

    if (blackCalibrated && !whiteCalibrated) {
        statusText.textContent = 'Black Reference Complete - White Reference Needed';
        mainCalibrationStatus.textContent = 'Partially Calibrated (1/3)';
    } else if (blackCalibrated && whiteCalibrated && !isCalibrated) {
        statusText.textContent = 'References Complete - Vivid White Calibration Needed';
        mainCalibrationStatus.textContent = 'Partially Calibrated (2/3)';
    }
}

function updateProfessionalProgress(blackCalibrated, whiteCalibrated, blueCalibrated, yellowCalibrated, isCalibrated) {
    const statusText = document.getElementById('calibrationStatusText');
    const mainCalibrationStatus = document.getElementById('calibrationStatus');

    const completedSteps = [blackCalibrated, whiteCalibrated, blueCalibrated, yellowCalibrated, isCalibrated].filter(Boolean).length;

    if (completedSteps === 0) {
        statusText.textContent = 'Professional 4-Point Calibration - Start with Black Reference';
        mainCalibrationStatus.textContent = 'Not Calibrated (0/5)';
    } else if (completedSteps === 1 && blackCalibrated) {
        statusText.textContent = 'Black Complete - White Reference Needed';
        mainCalibrationStatus.textContent = 'Professional Calibration (1/5)';
    } else if (completedSteps === 2 && whiteCalibrated) {
        statusText.textContent = 'Basic References Complete - Blue Reference Needed';
        mainCalibrationStatus.textContent = 'Professional Calibration (2/5)';
    } else if (completedSteps === 3 && blueCalibrated) {
        statusText.textContent = 'Blue Complete - Yellow Reference Needed';
        mainCalibrationStatus.textContent = 'Professional Calibration (3/5)';
    } else if (completedSteps === 4 && yellowCalibrated) {
        statusText.textContent = '4-Point References Complete - Vivid White Calibration Needed';
        mainCalibrationStatus.textContent = 'Professional Calibration (4/5)';
    } else if (completedSteps === 5) {
        statusText.textContent = 'Professional 4-Point Calibration Complete';
        mainCalibrationStatus.textContent = 'Professional Calibrated (5/5) - Tetrahedral';
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

    // Professional mode toggle
    document.getElementById('professionalModeToggle').addEventListener('change', function() {
        professionalMode = this.checked;
        toggleProfessionalMode(professionalMode);
    });

    // Blue reference calibration (Professional mode)
    document.getElementById('calibrateBlueBtn').addEventListener('click', async () => {
        const btn = document.getElementById('calibrateBlueBtn');
        btn.disabled = true;
        btn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Calibrating...';

        try {
            const response = await fetch('/api/calibrate-blue', { method: 'POST' });
            const data = await response.json();

            if (data.status === 'success') {
                showNotification('Blue reference calibrated successfully! Professional 4-point calibration in progress.', 'success');
                updateCalibrationStatus();
            } else {
                showNotification('Blue calibration failed: ' + (data.message || data.error || 'Unknown error'), 'error');
            }
        } catch (error) {
            showNotification('Blue calibration error: ' + error.message, 'error');
        } finally {
            btn.disabled = false;
            btn.innerHTML = '<i class="fas fa-square" style="color: #0066ff;"></i> Calibrate Blue Reference';
        }
    });

    // Yellow reference calibration (Professional mode)
    document.getElementById('calibrateYellowBtn').addEventListener('click', async () => {
        const btn = document.getElementById('calibrateYellowBtn');
        btn.disabled = true;
        btn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Calibrating...';

        try {
            const response = await fetch('/api/calibrate-yellow', { method: 'POST' });
            const data = await response.json();

            if (data.status === 'success') {
                showNotification('Yellow reference calibrated successfully! 4-point calibration complete - Tetrahedral interpolation enabled!', 'success');
                updateCalibrationStatus();
            } else {
                showNotification('Yellow calibration failed: ' + (data.message || data.error || 'Unknown error'), 'error');
            }
        } catch (error) {
            showNotification('Yellow calibration error: ' + error.message, 'error');
        } finally {
            btn.disabled = false;
            btn.innerHTML = '<i class="fas fa-square" style="color: #ffcc00;"></i> Calibrate Yellow Reference';
        }
    });

    // CIEDE2000 Validation (Professional mode)
    document.getElementById('validateCIEDE2000Btn').addEventListener('click', async () => {
        const btn = document.getElementById('validateCIEDE2000Btn');
        const resultsDiv = document.getElementById('validationResults');
        const statusDiv = document.getElementById('validationStatus');
        const threshold = document.getElementById('validationThreshold').value;

        btn.disabled = true;
        btn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Running Validation...';
        resultsDiv.style.display = 'block';
        statusDiv.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Running CIEDE2000 validation...';

        try {
            const response = await fetch(`/api/validate-ciede2000?threshold=${threshold}`);
            const data = await response.json();

            if (data.status === 'success') {
                displayValidationResults(data);
                showNotification('CIEDE2000 validation completed successfully!', 'success');
            } else {
                showNotification('Validation failed: ' + (data.message || data.error || 'Unknown error'), 'error');
                statusDiv.innerHTML = '<i class="fas fa-exclamation-triangle"></i> Validation failed';
            }
        } catch (error) {
            showNotification('Validation error: ' + error.message, 'error');
            statusDiv.innerHTML = '<i class="fas fa-exclamation-triangle"></i> Validation error';
        } finally {
            btn.disabled = false;
            btn.innerHTML = '<i class="fas fa-flask"></i> Run CIEDE2000 Validation';
        }
    });
}

// Professional mode toggle functionality
function toggleProfessionalMode(enabled) {
    const processTitle = document.getElementById('calibrationProcessTitle');
    const processHelp = document.getElementById('calibrationHelp');
    const blueStep = document.getElementById('blueCalibrationStep');
    const yellowStep = document.getElementById('yellowCalibrationStep');
    const vividWhiteStepNumber = document.getElementById('vividWhiteStepNumber');
    const validationPanel = document.getElementById('validationPanel');

    if (enabled) {
        // Enable professional 4-point calibration
        processTitle.innerHTML = '<i class="fas fa-graduation-cap"></i> Professional 4-Point Calibration Process';
        processHelp.textContent = 'Professional workflow with tetrahedral interpolation for industry-grade accuracy:';
        blueStep.style.display = 'block';
        yellowStep.style.display = 'block';
        vividWhiteStepNumber.textContent = '5';
        validationPanel.style.display = 'block';

        showNotification('Professional 4-Point Calibration enabled! This provides industry-grade color accuracy with tetrahedral interpolation.', 'success');
    } else {
        // Disable professional mode - standard 3-step calibration
        processTitle.innerHTML = '<i class="fas fa-list-ol"></i> 3-Step Calibration Process';
        processHelp.textContent = 'Follow these steps in a dark room for best results:';
        blueStep.style.display = 'none';
        yellowStep.style.display = 'none';
        vividWhiteStepNumber.textContent = '3';
        validationPanel.style.display = 'none';
    }

    // Update calibration status display
    updateCalibrationStatus();
}

// Display CIEDE2000 validation results
function displayValidationResults(data) {
    const statusDiv = document.getElementById('validationStatus');
    const avgDeltaE = document.getElementById('averageDeltaE');
    const maxDeltaE = document.getElementById('maxDeltaE');
    const testColorCount = document.getElementById('testColorCount');
    const qualityLevel = document.getElementById('qualityLevel');
    const recommendationsDiv = document.getElementById('validationRecommendations');

    const result = data.result;
    const passed = result.passed;

    // Update status
    statusDiv.innerHTML = `
        <i class="fas fa-${passed ? 'check-circle' : 'exclamation-triangle'}" style="color: ${passed ? '#10b981' : '#ef4444'};"></i>
        ${passed ? 'Validation PASSED' : 'Validation FAILED'} - ${result.qualityLevel}
    `;

    // Update metrics
    avgDeltaE.textContent = result.averageDeltaE.toFixed(3);
    avgDeltaE.style.color = result.averageDeltaE < 1.0 ? '#10b981' : result.averageDeltaE < 2.0 ? '#f59e0b' : '#ef4444';

    maxDeltaE.textContent = result.maxDeltaE.toFixed(3);
    maxDeltaE.style.color = result.maxDeltaE < 1.5 ? '#10b981' : result.maxDeltaE < 3.0 ? '#f59e0b' : '#ef4444';

    testColorCount.textContent = result.testColorCount;
    qualityLevel.textContent = result.qualityLevel;
    qualityLevel.style.color = passed ? '#10b981' : '#ef4444';

    // Update recommendations
    if (data.recommendations && data.recommendations.length > 0) {
        recommendationsDiv.innerHTML = `
            <h4><i class="fas fa-lightbulb"></i> Recommendations</h4>
            <ul>
                ${data.recommendations.map(rec => `<li>${rec}</li>`).join('')}
            </ul>
        `;
    } else {
        recommendationsDiv.innerHTML = '<h4><i class="fas fa-check"></i> No recommendations - calibration is optimal!</h4>';
    }
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

    // Live view shows RGB values only - color matching happens on capture
    const colorName = 'Live TCS3430 Reading';
    const displayTitle = 'Live TCS3430 Reading';

    container.innerHTML = `
        <div class="color-display">
            <div class="color-swatch live-swatch" style="background-color: ${hex};"></div>
            <div class="color-details">
                <p class="color-name">${displayTitle}</p>
                <p class="color-values">RGB: ${fastData.r}, ${fastData.g}, ${fastData.b}</p>
                <p class="color-values">HEX: ${hex}</p>
                <p class="color-values">Sensor: X=${fastData.x || 'N/A'}, Y=${fastData.y || 'N/A'}, Z=${fastData.z || 'N/A'}</p>
                <p class="color-values">IR: IR1=${fastData.ir1 || 'N/A'}, IR2=${fastData.ir2 || 'N/A'}</p>
                <p class="color-values" style="font-size: 0.9rem; color: #007bff; font-weight: 500;">
                    💡 Click "Capture Color" to identify color name
                </p>
                <p class="color-values" style="font-size: 0.8rem; opacity: 0.7;">
                    Updated: ${new Date().toLocaleTimeString()}
                </p>
            </div>
        </div>
    `;

    liveColor = {
        id: 0,
        name: colorName,
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
        // Call the new capture API that saves to storage
        const response = await fetch('/api/capture-color', {
            method: 'POST'
        });

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const captureData = await response.json();

        // Create captured color object with name (matching display function format)
        const hex = `#${captureData.rgb.r.toString(16).padStart(2, '0')}${captureData.rgb.g.toString(16).padStart(2, '0')}${captureData.rgb.b.toString(16).padStart(2, '0')}`;

        scannedColor = {
            name: captureData.colorName,
            rgb: {
                r: captureData.rgb.r,
                g: captureData.rgb.g,
                b: captureData.rgb.b
            },
            hex: hex,
            timestamp: captureData.timestamp,
            searchDuration: captureData.searchDuration,
            saved: captureData.saved
        };

        updateScannedColorDisplay(scannedColor);

        // Show appropriate notification based on save status
        if (captureData.saved) {
            showNotification(`Color captured and saved: ${captureData.colorName}`, 'success');
            // Refresh storage status after successful capture
            await fetchStorageStatus();
        } else {
            showNotification(`Color captured: ${captureData.colorName} (not saved to storage)`, 'warning');
        }

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

// Storage Management Functions
let storageData = {
    captures: [],
    status: null
};

// Fetch storage status
async function fetchStorageStatus() {
    try {
        const response = await fetch('/api/storage-status');
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const data = await response.json();
        storageData.status = data;
        updateStorageStatusDisplay();

    } catch (error) {
        console.error('Failed to fetch storage status:', error);
        showNotification('Failed to fetch storage status', 'error');
    }
}

// Update storage status display
function updateStorageStatusDisplay() {
    const status = storageData.status;
    if (!status) return;

    // Update capture count
    const capturesCount = document.getElementById('storedCapturesCount');
    const maxCaptures = document.getElementById('maxCaptures');
    if (capturesCount && status.captures) {
        capturesCount.textContent = status.captures.total || 0;
        if (maxCaptures) {
            maxCaptures.textContent = status.captures.max || 30;
        }
    }

    // Update storage usage
    const storageUsed = document.getElementById('storageUsed');
    if (storageUsed && status.storage) {
        const usagePercent = status.storage.usagePercent || 0;
        storageUsed.textContent = `${usagePercent.toFixed(1)}%`;

        // Update progress bar
        const progressFill = document.getElementById('storageProgressFill');
        if (progressFill) {
            progressFill.style.width = `${usagePercent}%`;
        }
    }

    // Update calibration status
    const calibrationStored = document.getElementById('calibrationStored');
    if (calibrationStored && status.calibration) {
        calibrationStored.textContent = status.calibration.hasData ? 'Saved' : 'None';
        calibrationStored.style.color = status.calibration.hasData ? '#10b981' : '#6b7280';
    }
}

// Fetch stored captures
async function fetchStoredCaptures() {
    try {
        const response = await fetch('/api/stored-captures');
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const data = await response.json();
        storageData.captures = data.captures || [];
        displayStoredCaptures();

    } catch (error) {
        console.error('Failed to fetch stored captures:', error);
        showNotification('Failed to fetch stored captures', 'error');
    }
}

// Display stored captures
function displayStoredCaptures() {
    const container = document.getElementById('storedCapturesContainer');
    const capturesList = document.getElementById('capturesList');

    if (!container || !capturesList) return;

    // Show the container
    container.style.display = 'block';

    if (storageData.captures.length === 0) {
        capturesList.innerHTML = `
            <div class="empty-captures">
                <i class="fas fa-inbox"></i>
                <p>No stored captures found</p>
                <p>Capture some colors to see them here!</p>
            </div>
        `;
        return;
    }

    // Sort captures by timestamp (newest first)
    const sortedCaptures = [...storageData.captures].sort((a, b) => b.timestamp - a.timestamp);

    capturesList.innerHTML = sortedCaptures.map((capture, index) => {
        const date = new Date(capture.timestamp * 1000);
        const timeStr = date.toLocaleString();
        const hexColor = capture.hex || rgbToHex(capture.rgb.r, capture.rgb.g, capture.rgb.b);

        return `
            <div class="capture-item" data-index="${capture.index}">
                <div class="capture-info">
                    <div class="capture-color-preview" style="background-color: ${hexColor}"></div>
                    <div class="capture-details">
                        <div class="capture-name">${capture.colorName}</div>
                        <div class="capture-meta">
                            RGB(${capture.rgb.r}, ${capture.rgb.g}, ${capture.rgb.b}) • ${timeStr}
                        </div>
                    </div>
                </div>
                <div class="capture-actions">
                    <button class="btn-small btn-danger" onclick="deleteCapture(${capture.index})">
                        <i class="fas fa-trash"></i>
                    </button>
                </div>
            </div>
        `;
    }).join('');
}

// Delete a specific capture
async function deleteCapture(index) {
    if (!confirm('Are you sure you want to delete this capture?')) {
        return;
    }

    try {
        const response = await fetch(`/api/delete-capture?index=${index}`, {
            method: 'DELETE'
        });

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const data = await response.json();
        showNotification(data.message, data.status === 'success' ? 'success' : 'error');

        if (data.status === 'success') {
            // Refresh the captures list and storage status
            await fetchStoredCaptures();
            await fetchStorageStatus();
        }

    } catch (error) {
        console.error('Failed to delete capture:', error);
        showNotification('Failed to delete capture', 'error');
    }
}

// Clear all captures
async function clearAllCaptures() {
    if (!confirm('Are you sure you want to clear ALL stored captures? This cannot be undone.')) {
        return;
    }

    try {
        const response = await fetch('/api/clear-captures', {
            method: 'POST'
        });

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const data = await response.json();
        showNotification(data.message, data.status === 'success' ? 'success' : 'error');

        if (data.status === 'success') {
            // Clear the display and refresh status
            storageData.captures = [];
            displayStoredCaptures();
            await fetchStorageStatus();
        }

    } catch (error) {
        console.error('Failed to clear captures:', error);
        showNotification('Failed to clear captures', 'error');
    }
}

// Export data
async function exportData() {
    try {
        const response = await fetch('/api/export-captures');
        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const data = await response.json();

        // Create and download the file
        const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `color-captures-export-${new Date().toISOString().split('T')[0]}.json`;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);

        showNotification('Data exported successfully', 'success');

    } catch (error) {
        console.error('Failed to export data:', error);
        showNotification('Failed to export data', 'error');
    }
}

// Setup storage management handlers
function setupStorageHandlers() {
    // View stored captures button
    const viewCapturesBtn = document.getElementById('viewStoredCapturesBtn');
    if (viewCapturesBtn) {
        viewCapturesBtn.addEventListener('click', async () => {
            await fetchStoredCaptures();
        });
    }

    // Export data button
    const exportBtn = document.getElementById('exportDataBtn');
    if (exportBtn) {
        exportBtn.addEventListener('click', exportData);
    }

    // Clear captures button
    const clearBtn = document.getElementById('clearCapturesBtn');
    if (clearBtn) {
        clearBtn.addEventListener('click', clearAllCaptures);
    }

    // Refresh storage button
    const refreshBtn = document.getElementById('refreshStorageBtn');
    if (refreshBtn) {
        refreshBtn.addEventListener('click', async () => {
            await fetchStorageStatus();
            showNotification('Storage status refreshed', 'success');
        });
    }
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

    // Set up storage management handlers
    setupStorageHandlers();

    // Start fetching data immediately
    fetchFastColor();
    fetchBatteryStatus();
    fetchStorageStatus();

    // Set up fast polling for sensor data (75ms for very smooth real-time updates)
    setInterval(fetchFastColor, 75);

    // Set up battery monitoring (update every 10 seconds)
    setInterval(fetchBatteryStatus, 10000);

    // Set up storage status monitoring (update every 30 seconds)
    setInterval(fetchStorageStatus, 30000);

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
