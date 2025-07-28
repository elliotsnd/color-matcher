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

// Enhanced calibration status tracking
let calibrationStatus = {
    isCalibrated: false,
    blackReference: null,
    whiteReference: null,
    vividWhiteCalibrated: false,
    greyCalibrated: false,
    // Professional 5-point calibration
    is5PointCalibrated: false,
    blueReference: null,
    yellowReference: null,
    interpolationMethod: 'linear',
    // FINAL 6 COLORS ONLY - no extended colors needed
    // Enhanced status
    totalPoints: 0,
    progress: 0,
    calibrationTier: 3,
    tierDescription: 'Uncalibrated - Basic functionality'
};

// Auto-calibration state
let autoCalibrationState = {
    isActive: false,
    currentStep: 0,
    totalSteps: 11,
    currentColor: '',
    targetRGB: { r: 0, g: 0, b: 0 },
    canSkip: false,
    instructions: ''
};

// Manual calibration mode state
let manualCalibrationMode = false;

// Update enhanced calibration status display
function updateCalibrationStatus() {
    // Fetch enhanced calibration status
    fetch('/api/enhanced-calibration-status')
        .then(response => response.json())
        .then(data => {
            calibrationStatus = { ...calibrationStatus, ...data };

            // Update main status indicator
            const indicator = document.getElementById('calibrationIndicator');
            const statusText = document.getElementById('calibrationStatusText');
            const mainCalibrationStatus = document.getElementById('calibrationStatus');

            if (data.calibration_complete) {
                indicator.className = 'fas fa-circle status-good';
                statusText.textContent = 'Calibration Complete';
                mainCalibrationStatus.textContent = 'Calibrated ✓';
            } else {
                indicator.className = 'fas fa-circle status-warning';
                statusText.textContent = 'Calibration Required';
                mainCalibrationStatus.textContent = 'Not Calibrated';
            }

            // Update tier status
            updateTierStatus(data.calibration_tier);

            // Update progress
            updateCalibrationProgress(data.progress || 0);

            // Update professional calibration status
            if (data.ccm_valid) {
                calibrationStatus.is5PointCalibrated = data.calibration_complete;
                calibrationStatus.interpolationMethod = 'matrix-based';
            }

            // Update individual step status for 6 colors only
            if (data.core_calibration) {
                updateStepStatus('blackStatus', data.core_calibration.black_calibrated);
                updateStepStatus('vividWhiteStatus', data.core_calibration.white_calibrated);
                updateStepStatus('redStatus', data.core_calibration.red_calibrated);
                updateStepStatus('greenStatus', data.core_calibration.green_calibrated);
                updateStepStatus('blueStatus', data.core_calibration.blue_calibrated);
                updateStepStatus('yellowStatus', data.core_calibration.yellow_calibrated);
            }

            // REMOVED: Extended color status updates - using only 6 core colors

            // REMOVED: Professional calibration status updates (not needed for 6-color system)

            // Update tier status display
            if (data.calibration_tier) {
                const tierBadge = document.getElementById('tierBadge');
                const tierDescription = document.getElementById('tierDescription');

                if (tierBadge && tierDescription) {
                    tierBadge.textContent = data.calibration_tier.current_tier;
                    tierBadge.className = `tier-badge tier-${data.calibration_tier.tier_level}`;
                    tierDescription.textContent = data.calibration_tier.description;
                }
            }
        })
        .catch(error => {
            console.error('Failed to get enhanced calibration status:', error);
            // Fallback to basic calibration status
            fetch('/api/calibration-status')
                .then(response => response.json())
                .then(data => {
                    updateBasicCalibrationStatus(data);
                })
                .catch(fallbackError => {
                    console.error('Failed to get basic calibration status:', fallbackError);
                    document.getElementById('calibrationStatusText').textContent = 'Status Check Failed';
                });
        });
}

// Update tier status display
function updateTierStatus(tierData) {
    const tierBadge = document.getElementById('tierBadge');
    const tierDescription = document.getElementById('tierDescription');

    if (tierData && tierBadge && tierDescription) {
        tierBadge.textContent = tierData.current_tier;
        tierBadge.className = `tier-badge tier-${tierData.tier_level}`;
        tierDescription.textContent = tierData.description;
    }
}

// Update calibration progress
function updateCalibrationProgress(progress) {
    const progressFill = document.getElementById('calibrationProgress');
    const progressText = document.getElementById('progressText');

    if (progressFill && progressText) {
        progressFill.style.width = `${progress}%`;
        progressText.textContent = `${progress}% Complete`;
    }
}

// Fallback function for basic calibration status
function updateBasicCalibrationStatus(data) {
    const indicator = document.getElementById('calibrationIndicator');
    const statusText = document.getElementById('calibrationStatusText');
    const mainCalibrationStatus = document.getElementById('calibrationStatus');

    if (data.is_complete) {
        indicator.className = 'fas fa-circle status-good';
        statusText.textContent = 'Calibration Complete';
        mainCalibrationStatus.textContent = 'Calibrated ✓';
    } else {
        indicator.className = 'fas fa-circle status-warning';
        statusText.textContent = 'Calibration Required';
        mainCalibrationStatus.textContent = 'Not Calibrated';
    }

    // Update basic step status for 6 colors only
    updateStepStatus('blackStatus', data.black_calibrated);
    updateStepStatus('vividWhiteStatus', data.white_calibrated);
    updateStepStatus('redStatus', data.red_calibrated);
    updateStepStatus('greenStatus', data.green_calibrated);
    updateStepStatus('blueStatus', data.blue_calibrated);
    updateStepStatus('yellowStatus', data.yellow_calibrated);
}

function updateStepStatus(elementId, isComplete) {
    const element = document.getElementById(elementId);
    if (element) {
        element.textContent = isComplete ? 'Completed ✓' : 'Not calibrated';
        element.className = isComplete ? 'step-status completed' : 'step-status pending';
    }
}

// Auto-calibration functions
function startAutoCalibration() {
    fetch('/api/start-auto-calibration', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        }
    })
    .then(response => response.json())
    .then(data => {
        if (data.status === 'success') {
            autoCalibrationState.isActive = true;
            showAutoCalibrationProgress();
            updateAutoCalibrationStatus();
            showNotification('Auto-calibration started!', 'success');
        } else {
            showNotification('Failed to start auto-calibration: ' + data.message, 'error');
        }
    })
    .catch(error => {
        console.error('Error starting auto-calibration:', error);
        showNotification('Error starting auto-calibration', 'error');
    });
}

function updateAutoCalibrationStatus() {
    if (!autoCalibrationState.isActive) return;

    fetch('/api/auto-calibration-status')
        .then(response => response.json())
        .then(data => {
            if (data.status === 'success') {
                autoCalibrationState.currentStep = data.current_step;
                autoCalibrationState.totalSteps = data.total_steps;
                autoCalibrationState.currentColor = data.current_color;
                autoCalibrationState.targetRGB = {
                    r: data.target_r,
                    g: data.target_g,
                    b: data.target_b
                };
                autoCalibrationState.canSkip = data.can_skip;
                autoCalibrationState.instructions = data.instructions;

                updateAutoCalibrationDisplay();

                // Check if completed
                if (data.auto_calibration_state === 3) { // COMPLETED
                    autoCalibrationComplete();
                }
            }
        })
        .catch(error => {
            console.error('Error getting auto-calibration status:', error);
        });
}

function updateAutoCalibrationDisplay() {
    // Update current color name
    const colorNameElement = document.getElementById('currentColorName');
    if (colorNameElement) {
        colorNameElement.textContent = autoCalibrationState.currentColor;
    }

    // Update color swatch
    const colorSwatchElement = document.getElementById('currentColorSwatch');
    if (colorSwatchElement) {
        const rgb = autoCalibrationState.targetRGB;
        colorSwatchElement.style.backgroundColor = `rgb(${rgb.r}, ${rgb.g}, ${rgb.b})`;
    }

    // Update target RGB
    const targetRGBElement = document.getElementById('targetRGB');
    if (targetRGBElement) {
        const rgb = autoCalibrationState.targetRGB;
        targetRGBElement.textContent = `RGB(${rgb.r}, ${rgb.g}, ${rgb.b})`;
    }

    // Update step counter
    const stepCounterElement = document.getElementById('stepCounter');
    if (stepCounterElement) {
        stepCounterElement.textContent = `Step ${autoCalibrationState.currentStep} of ${autoCalibrationState.totalSteps}`;
    }

    // Update step progress
    const stepProgressFill = document.getElementById('stepProgressFill');
    if (stepProgressFill) {
        const progress = ((autoCalibrationState.currentStep - 1) / autoCalibrationState.totalSteps) * 100;
        stepProgressFill.style.width = `${progress}%`;
    }

    // Update instructions
    const instructionsElement = document.getElementById('autoCalInstructions');
    if (instructionsElement) {
        instructionsElement.textContent = autoCalibrationState.instructions;
    }

    // Update skip button state
    const skipButton = document.getElementById('autoCalSkipBtn');
    if (skipButton) {
        skipButton.disabled = !autoCalibrationState.canSkip;
    }
}

function autoCalibrationNext() {
    fetch('/api/auto-calibration-next', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        }
    })
    .then(response => response.json())
    .then(data => {
        if (data.status === 'success') {
            updateAutoCalibrationStatus();
            showNotification('Advanced to next color', 'success');
        } else {
            showNotification('Failed to advance: ' + data.message, 'error');
        }
    })
    .catch(error => {
        console.error('Error advancing auto-calibration:', error);
        showNotification('Error advancing auto-calibration', 'error');
    });
}

function autoCalibrationRetry() {
    fetch('/api/auto-calibration-retry', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        }
    })
    .then(response => response.json())
    .then(data => {
        if (data.status === 'success') {
            updateAutoCalibrationStatus();
            showNotification('Retrying current color', 'info');
        } else {
            showNotification('Failed to retry: ' + data.message, 'error');
        }
    })
    .catch(error => {
        console.error('Error retrying auto-calibration:', error);
        showNotification('Error retrying auto-calibration', 'error');
    });
}

function autoCalibrationSkip() {
    fetch('/api/auto-calibration-skip', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
        }
    })
    .then(response => response.json())
    .then(data => {
        if (data.status === 'success') {
            updateAutoCalibrationStatus();
            showNotification('Skipped current color', 'info');
        } else {
            showNotification('Failed to skip: ' + data.message, 'error');
        }
    })
    .catch(error => {
        console.error('Error skipping auto-calibration:', error);
        showNotification('Error skipping auto-calibration', 'error');
    });
}

function autoCalibrationCancel() {
    autoCalibrationState.isActive = false;
    hideAutoCalibrationProgress();
    showNotification('Auto-calibration cancelled', 'info');
}

function autoCalibrationComplete() {
    autoCalibrationState.isActive = false;
    hideAutoCalibrationProgress();
    updateCalibrationStatus(); // Refresh overall status
    showNotification('Auto-calibration completed successfully!', 'success');
}

function showAutoCalibrationProgress() {
    const autoCalProgress = document.getElementById('autoCalProgress');
    const startButton = document.getElementById('startAutoCalibrationBtn');

    if (autoCalProgress) {
        autoCalProgress.style.display = 'block';
    }
    if (startButton) {
        startButton.style.display = 'none';
    }
}

function hideAutoCalibrationProgress() {
    const autoCalProgress = document.getElementById('autoCalProgress');
    const startButton = document.getElementById('startAutoCalibrationBtn');

    if (autoCalProgress) {
        autoCalProgress.style.display = 'none';
    }
    if (startButton) {
        startButton.style.display = 'block';
    }
}

// Check if auto-calibration is already active on page load
function checkAutoCalibrationStatus() {
    fetch('/api/auto-calibration-status')
        .then(response => response.json())
        .then(data => {
            if (data.status === 'success' && data.auto_calibration_state > 0) {
                // Auto-calibration is active
                autoCalibrationState.isActive = true;
                autoCalibrationState.currentStep = data.current_step;
                autoCalibrationState.totalSteps = data.total_steps;
                autoCalibrationState.currentColor = data.current_color;
                autoCalibrationState.targetRGB = {
                    r: data.target_r,
                    g: data.target_g,
                    b: data.target_b
                };
                autoCalibrationState.canSkip = data.can_skip;
                autoCalibrationState.instructions = data.instructions;

                showAutoCalibrationProgress();
                updateAutoCalibrationDisplay();
            }
        })
        .catch(error => {
            console.error('Error checking auto-calibration status:', error);
        });
}

function updateStandardProgress(blackCalibrated, whiteCalibrated, isComplete) {
    const statusText = document.getElementById('calibrationStatusText');
    const mainCalibrationStatus = document.getElementById('calibrationStatus');

    if (blackCalibrated && !whiteCalibrated) {
        statusText.textContent = 'Black Reference Complete - White Reference Needed';
        mainCalibrationStatus.textContent = 'Partially Calibrated (1/5)';
    } else if (blackCalibrated && whiteCalibrated && !isComplete) {
        statusText.textContent = 'References Complete - Grey Point Needed';
        mainCalibrationStatus.textContent = 'Partially Calibrated (2/5)';
    }
}

function updateProfessionalProgress(blackCalibrated, whiteCalibrated, blueCalibrated, yellowCalibrated, isComplete) {
    const statusText = document.getElementById('calibrationStatusText');
    const mainCalibrationStatus = document.getElementById('calibrationStatus');

    const completedSteps = [blackCalibrated, whiteCalibrated, blueCalibrated, yellowCalibrated, isComplete].filter(Boolean).length;

    if (completedSteps === 0) {
        statusText.textContent = 'Professional 5-Point Calibration - Start with Black Reference';
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
        statusText.textContent = '4-Point References Complete - Grey Point Needed';
        mainCalibrationStatus.textContent = 'Professional Calibration (4/5)';
    } else if (completedSteps === 5) {
        statusText.textContent = 'Professional 5-Point Matrix Calibration Complete';
        mainCalibrationStatus.textContent = 'Professional Calibrated (5/5) - Matrix-Based';
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
            const response = await fetch('/api/calibrate-black', { method: 'POST' });
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
            btn.innerHTML = '<i class="fas fa-square"></i> Calibrate Black';
        }
    });

    // REMOVED: White calibration handler (replaced with vivid white)

    // Red reference calibration
    document.getElementById('calibrateRedBtn').addEventListener('click', async () => {
        const btn = document.getElementById('calibrateRedBtn');
        btn.disabled = true;
        btn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Calibrating...';

        try {
            const response = await fetch('/api/calibrate-red', { method: 'POST' });
            const data = await response.json();

            if (data.status === 'success') {
                showNotification('Red reference calibrated successfully!', 'success');
                updateCalibrationStatus();
            } else {
                showNotification('Red calibration failed: ' + (data.message || data.error || 'Unknown error'), 'error');
            }
        } catch (error) {
            showNotification('Red calibration error: ' + error.message, 'error');
        } finally {
            btn.disabled = false;
            btn.innerHTML = '<i class="fas fa-square"></i> Calibrate Red';
        }
    });

    // Green reference calibration
    document.getElementById('calibrateGreenBtn').addEventListener('click', async () => {
        const btn = document.getElementById('calibrateGreenBtn');
        btn.disabled = true;
        btn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Calibrating...';

        try {
            const response = await fetch('/api/calibrate-green', { method: 'POST' });
            const data = await response.json();

            if (data.status === 'success') {
                showNotification('Green reference calibrated successfully!', 'success');
                updateCalibrationStatus();
            } else {
                showNotification('Green calibration failed: ' + (data.message || data.error || 'Unknown error'), 'error');
            }
        } catch (error) {
            showNotification('Green calibration error: ' + error.message, 'error');
        } finally {
            btn.disabled = false;
            btn.innerHTML = '<i class="fas fa-square"></i> Calibrate Green';
        }
    });

    // Manual mode toggle (removed - handled in auto-calibration section)
    // The manual mode toggle is now handled in the auto-calibration event listeners section

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
            btn.innerHTML = '<i class="fas fa-square"></i> Calibrate Blue';
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
            btn.innerHTML = '<i class="fas fa-square"></i> Calibrate Yellow';
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

    // Auto-calibration event listeners
    const startAutoCalBtn = document.getElementById('startAutoCalibrationBtn');
    if (startAutoCalBtn) {
        startAutoCalBtn.addEventListener('click', startAutoCalibration);
    }

    const autoCalNextBtn = document.getElementById('autoCalNextBtn');
    if (autoCalNextBtn) {
        autoCalNextBtn.addEventListener('click', autoCalibrationNext);
    }

    const autoCalRetryBtn = document.getElementById('autoCalRetryBtn');
    if (autoCalRetryBtn) {
        autoCalRetryBtn.addEventListener('click', autoCalibrationRetry);
    }

    const autoCalSkipBtn = document.getElementById('autoCalSkipBtn');
    if (autoCalSkipBtn) {
        autoCalSkipBtn.addEventListener('click', autoCalibrationSkip);
    }

    const autoCalCancelBtn = document.getElementById('autoCalCancelBtn');
    if (autoCalCancelBtn) {
        autoCalCancelBtn.addEventListener('click', autoCalibrationCancel);
    }

    // Manual calibration mode toggle
    const manualModeToggle = document.getElementById('manualModeToggle');
    if (manualModeToggle) {
        manualModeToggle.addEventListener('change', (e) => {
            manualCalibrationMode = e.target.checked;
            toggleManualCalibrationMode(manualCalibrationMode);
        });
    }

    // REMOVED: Professional two-stage calibration handlers (not needed for 6-color system)

    // Vivid white calibration handler (replaces white)
    setupVividWhiteHandler();

    // Extended color calibration handlers (now empty)
    setupExtendedColorHandlers();
}

// REMOVED: Professional two-stage calibration handlers (not needed for 6-color system)

// Setup vivid white calibration handler (replaces white)
function setupVividWhiteHandler() {
    const vividWhiteBtn = document.getElementById('calibrateVividWhiteBtn');
    if (vividWhiteBtn) {
        vividWhiteBtn.addEventListener('click', () => {
            calibrateColor('/api/calibrate-vivid-white', 'Vivid White');
        });
    }
}

// REMOVED: Extended color handlers - using only 6 core colors
function setupExtendedColorHandlers() {
    // No extended colors needed - all 6 colors are core colors
    const extendedColors = [];
    // REMOVED: All extended color handlers

    extendedColors.forEach(color => {
        const btn = document.getElementById(color.id);
        if (btn) {
            btn.addEventListener('click', async () => {
                btn.disabled = true;
                btn.innerHTML = '<i class="fas fa-spinner fa-spin"></i> Calibrating...';

                try {
                    const response = await fetch(color.endpoint, { method: 'POST' });
                    const data = await response.json();

                    if (data.status === 'success') {
                        showNotification(`${color.name} calibrated successfully!`, 'success');
                        updateCalibrationStatus();
                    } else {
                        showNotification(`${color.name} calibration failed: ${data.message || 'Unknown error'}`, 'error');
                    }
                } catch (error) {
                    showNotification(`${color.name} calibration error: ${error.message}`, 'error');
                } finally {
                    btn.disabled = false;
                    btn.innerHTML = `<i class="fas fa-square"></i> Calibrate ${color.name}`;
                }
            });
        }
    });
}

// Toggle manual calibration mode
function toggleManualCalibrationMode(enabled) {
    const manualSteps = document.getElementById('manualCalibrationSteps');

    if (manualSteps) {
        manualSteps.style.display = enabled ? 'block' : 'none';
    }
}

// Professional mode toggle functionality
function toggleProfessionalMode(enabled) {
    const processTitle = document.getElementById('calibrationProcessTitle');
    const processHelp = document.getElementById('calibrationHelp');
    const blueStep = document.getElementById('blueCalibrationStep');
    const yellowStep = document.getElementById('yellowCalibrationStep');
    const vividWhiteStepNumber = document.getElementById('vividWhiteStepNumber');
    const greyStepNumber = document.getElementById('greyStepNumber');
    const validationPanel = document.getElementById('validationPanel');

    if (enabled) {
        // Enable professional 4-point calibration
        processTitle.innerHTML = '<i class="fas fa-graduation-cap"></i> Professional 4-Point Calibration Process';
        processHelp.textContent = 'Professional workflow with tetrahedral interpolation for industry-grade accuracy:';
        blueStep.style.display = 'block';
        yellowStep.style.display = 'block';
        vividWhiteStepNumber.textContent = '5';
        if (greyStepNumber) greyStepNumber.textContent = '5';
        validationPanel.style.display = 'block';

        showNotification('Professional 4-Point Calibration enabled! This provides industry-grade color accuracy with tetrahedral interpolation.', 'success');
    } else {
        // Disable professional mode - standard 3-step calibration
        processTitle.innerHTML = '<i class="fas fa-list-ol"></i> 3-Step Calibration Process';
        processHelp.textContent = 'Follow these steps in a dark room for best results:';
        blueStep.style.display = 'none';
        yellowStep.style.display = 'none';
        vividWhiteStepNumber.textContent = '3';
        if (greyStepNumber) greyStepNumber.textContent = '3';
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
            const response = await fetch('/api/calibration-status');
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

    // Check if auto-calibration is already active
    checkAutoCalibrationStatus();

    // Set up auto-calibration status polling (every 2 seconds when active)
    setInterval(() => {
        if (autoCalibrationState.isActive) {
            updateAutoCalibrationStatus();
        }
    }, 2000);

    console.log('TCS3430 Calibration System initialized successfully!');
}

// Start the application when DOM is loaded
if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
} else {
    init();
}
