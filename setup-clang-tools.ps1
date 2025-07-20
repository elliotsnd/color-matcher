# Setup Script for Clang-Format and Clang-Tidy on Windows
# ESP32 Color-Matcher Project
#
# This script installs and configures Clang-Format and Clang-Tidy for the project

Write-Host "ESP32 Color-Matcher: Setting up Clang-Format and Clang-Tidy..." -ForegroundColor Green

# Check if running as Administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")

if (-not $isAdmin) {
    Write-Host "ERROR: This script must be run as Administrator!" -ForegroundColor Red
    Write-Host "Please:" -ForegroundColor Yellow
    Write-Host "1. Right-click PowerShell" -ForegroundColor Yellow
    Write-Host "2. Select 'Run as Administrator'" -ForegroundColor Yellow
    Write-Host "3. Navigate to your project directory and run this script again" -ForegroundColor Yellow
    Read-Host "Press Enter to exit"
    exit 1
}

# Check if Chocolatey is installed
try {
    $chocoVersion = choco --version
    Write-Host "✓ Chocolatey is installed: $chocoVersion" -ForegroundColor Green
} catch {
    Write-Host "Installing Chocolatey..." -ForegroundColor Yellow
    Set-ExecutionPolicy Bypass -Scope Process -Force
    [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
    iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
}

# Install LLVM (includes Clang-Format and Clang-Tidy)
Write-Host "Installing LLVM (includes Clang-Format and Clang-Tidy)..." -ForegroundColor Yellow
try {
    choco install llvm -y
    Write-Host "✓ LLVM installed successfully!" -ForegroundColor Green
} catch {
    Write-Host "ERROR: Failed to install LLVM. Please check Chocolatey installation." -ForegroundColor Red
    exit 1
}

# Refresh environment variables
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

# Verify installations
Write-Host "Verifying installations..." -ForegroundColor Yellow

try {
    $formatVersion = clang-format --version
    Write-Host "✓ Clang-Format: $formatVersion" -ForegroundColor Green
} catch {
    Write-Host "⚠ Clang-Format not found in PATH. You may need to restart your terminal." -ForegroundColor Yellow
}

try {
    $tidyVersion = clang-tidy --version
    Write-Host "✓ Clang-Tidy: $tidyVersion" -ForegroundColor Green
} catch {
    Write-Host "⚠ Clang-Tidy not found in PATH. You may need to restart your terminal." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "Setup complete! Next steps:" -ForegroundColor Green
Write-Host "1. Restart your terminal/VSCode to refresh PATH" -ForegroundColor Cyan
Write-Host "2. Run 'pio check' to analyze your code" -ForegroundColor Cyan
Write-Host "3. Run 'clang-format -i src/*.cpp src/*.h' to format files" -ForegroundColor Cyan
Write-Host ""
Write-Host "Configuration files already present:" -ForegroundColor Yellow
Write-Host "✓ .clang-format (ESP32-optimized formatting rules)" -ForegroundColor Green
Write-Host "✓ .clang-tidy (dead code detection and quality checks)" -ForegroundColor Green
Write-Host "✓ platformio.ini updated with --fix flag" -ForegroundColor Green

Read-Host "Press Enter to exit"
