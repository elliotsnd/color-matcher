#!/usr/bin/env pwsh

Write-Host "🔄 Re-checking PVS-Studio Extension..." -ForegroundColor Cyan

# Wait a moment for extension to register
Start-Sleep -Seconds 2

# Check if extension is now available
try {
    $extensions = & code --list-extensions 2>$null
    if ($extensions -contains "evgeniyryzhkov.pvs-studio-vscode") {
        Write-Host "✅ PVS-Studio extension is now installed!" -ForegroundColor Green
    } else {
        Write-Host "❌ PVS-Studio extension still not found" -ForegroundColor Red
        Write-Host "📋 Please install manually via Extensions panel (Ctrl+Shift+X)" -ForegroundColor Yellow
        Write-Host "Search for: 'PVS-Studio'" -ForegroundColor Yellow
    }
} catch {
    Write-Host "⚠️ Could not verify extension status" -ForegroundColor Yellow
}

Write-Host "`n🎯 AFTER INSTALLING THE EXTENSION:" -ForegroundColor Cyan
Write-Host "1. ⭐ COMPLETELY CLOSE VS Code/Cursor" -ForegroundColor White
Write-Host "2. ⭐ REOPEN in this folder" -ForegroundColor White  
Write-Host "3. ⭐ Open src/main.cpp" -ForegroundColor White
Write-Host "4. Press Ctrl+Shift+P" -ForegroundColor White
Write-Host "5. Type: 'PVS-Studio: Analyze Current File'" -ForegroundColor White
Write-Host "6. The 'no suitable targets' error should be resolved! ✨" -ForegroundColor Green
