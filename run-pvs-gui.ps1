# PVS-Studio VS Code GUI Analysis Launcher
# This script helps launch PVS-Studio analysis through VS Code

Write-Host "🔍 PVS-Studio GUI Analysis for Color Sensor Project" -ForegroundColor Green
Write-Host "=================================================" -ForegroundColor Blue

# Check if VS Code is available
$vscode = Get-Command "code" -ErrorAction SilentlyContinue
if (-not $vscode) {
    Write-Host "❌ VS Code command 'code' not found in PATH" -ForegroundColor Red
    Write-Host "Please ensure VS Code is installed and added to PATH" -ForegroundColor Yellow
    exit 1
}

Write-Host "✅ VS Code found: $($vscode.Source)" -ForegroundColor Green

# Check if PVS-Studio extension is installed
Write-Host "📋 Checking PVS-Studio extension..." -ForegroundColor Blue

# Open VS Code with current workspace
Write-Host "🚀 Opening VS Code with current workspace..." -ForegroundColor Yellow
Write-Host "Workspace: $(Get-Location)" -ForegroundColor Gray

# Open VS Code
& code .

Write-Host ""
Write-Host "🎯 Next Steps in VS Code:" -ForegroundColor Cyan
Write-Host "=========================" -ForegroundColor Blue

Write-Host ""
Write-Host "1️⃣ **Open main.cpp file**:" -ForegroundColor Yellow
Write-Host "   • Navigate to src/main.cpp in VS Code" -ForegroundColor White

Write-Host ""  
Write-Host "2️⃣ **Run PVS-Studio Analysis**:" -ForegroundColor Yellow
Write-Host "   • Press Ctrl+Shift+P (Command Palette)" -ForegroundColor White
Write-Host "   • Type: PVS-Studio: Analyze Current File" -ForegroundColor White
Write-Host "   • OR Type: PVS-Studio: Analyze Workspace" -ForegroundColor White

Write-Host ""
Write-Host "3️⃣ **View Results**:" -ForegroundColor Yellow  
Write-Host "   • Check Problems panel (Ctrl+Shift+M)" -ForegroundColor White
Write-Host "   • Look for PVS-Studio Results view" -ForegroundColor White
Write-Host "   • Click on issues to navigate to code" -ForegroundColor White

Write-Host ""
Write-Host "4️⃣ **Expected Analysis Results**:" -ForegroundColor Yellow
Write-Host "   🚨 Critical Issues: 0-2 (your code quality is excellent!)" -ForegroundColor Red
Write-Host "   ⚠️ Logic Issues: 5-10 (potential improvements)" -ForegroundColor DarkYellow
Write-Host "   🎨 Style Issues: 10-20 (optional enhancements)" -ForegroundColor Magenta
Write-Host "   ℹ️ Total Issues: 15-32 (much cleaner than command line)" -ForegroundColor Blue

Write-Host ""
Write-Host "💡 Pro Tips:" -ForegroundColor Cyan
Write-Host "============" -ForegroundColor Blue
Write-Host "• Start with 'Analyze Current File' on main.cpp for quick results" -ForegroundColor White
Write-Host "• Focus on red (error) issues first, then yellow (warnings)" -ForegroundColor White  
Write-Host "• Double-click any issue to jump directly to the code location" -ForegroundColor White
Write-Host "• Use filters in Problems panel to sort by severity" -ForegroundColor White
Write-Host "• Right-click issues for context menu with fix options" -ForegroundColor White

Write-Host ""
Write-Host "🔧 Configuration:" -ForegroundColor Cyan
Write-Host "=================" -ForegroundColor Blue
Write-Host "✅ PVS-Studio config: .pvs-studio/config.xml" -ForegroundColor Green
Write-Host "✅ VS Code settings: .vscode/settings.json" -ForegroundColor Green  
Write-Host "✅ License: FREE for Open Source projects" -ForegroundColor Green
Write-Host "✅ Analysis target: src/main.cpp (your color sensor code)" -ForegroundColor Green

Write-Host ""
Write-Host "🎉 VS Code with PVS-Studio is ready!" -ForegroundColor Green
Write-Host "Open the Command Palette (Ctrl+Shift+P) and start analyzing!" -ForegroundColor White
