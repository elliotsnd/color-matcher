#!/usr/bin/env pwsh

Write-Host "🔍 PVS-Studio Setup Verification" -ForegroundColor Cyan
Write-Host "=================================" -ForegroundColor Cyan

# Check compilation database
Write-Host "`n1️⃣ Checking compile_commands.json..." -ForegroundColor Yellow
if (Test-Path "compile_commands.json") {
    $compileDB = Get-Content "compile_commands.json" | ConvertFrom-Json
    Write-Host "✅ Found compile_commands.json with $($compileDB.Count) entries" -ForegroundColor Green
    
    # Show first entry that contains main.cpp
    $mainEntry = $compileDB | Where-Object { $_.file -like "*main.cpp*" }
    if ($mainEntry) {
        Write-Host "✅ Found main.cpp entry in compilation database" -ForegroundColor Green
    } else {
        Write-Host "❌ main.cpp not found in compilation database" -ForegroundColor Red
    }
} else {
    Write-Host "❌ compile_commands.json not found!" -ForegroundColor Red
}

# Check VS Code settings
Write-Host "`n2️⃣ Checking VS Code settings..." -ForegroundColor Yellow
if (Test-Path ".vscode/settings.json") {
    $settings = Get-Content ".vscode/settings.json" | ConvertFrom-Json
    Write-Host "✅ Found .vscode/settings.json" -ForegroundColor Green
    
    if ($settings."pvs-studio.analyzer.licenseType" -eq "free") {
        Write-Host "✅ License type: $($settings.'pvs-studio.analyzer.licenseType')" -ForegroundColor Green
    }
    
    if ($settings."pvs-studio.analyzer.projectType" -eq "compile_commands") {
        Write-Host "✅ Project type: $($settings.'pvs-studio.analyzer.projectType')" -ForegroundColor Green
    }
    
    $compileCommandsPath = $settings."pvs-studio.analyzer.compileCommandsPath"
    if ($compileCommandsPath) {
        Write-Host "✅ Compile commands path: $compileCommandsPath" -ForegroundColor Green
    }
} else {
    Write-Host "❌ .vscode/settings.json not found!" -ForegroundColor Red
}

# Check PVS-Studio project file
Write-Host "`n3️⃣ Checking PVS-Studio project file..." -ForegroundColor Yellow
if (Test-Path ".pvs-studio/project.json") {
    Write-Host "✅ Found .pvs-studio/project.json" -ForegroundColor Green
} else {
    Write-Host "❌ .pvs-studio/project.json not found!" -ForegroundColor Red
}

# Check if PVS-Studio extension is installed
Write-Host "`n4️⃣ Checking PVS-Studio extension..." -ForegroundColor Yellow
try {
    $extensions = & code --list-extensions 2>$null
    if ($extensions -contains "evgeniyryzhkov.pvs-studio-vscode") {
        Write-Host "✅ PVS-Studio extension is installed" -ForegroundColor Green
    } else {
        Write-Host "❌ PVS-Studio extension not found in installed extensions" -ForegroundColor Red
        Write-Host "📋 Installed extensions:" -ForegroundColor Gray
        $extensions | ForEach-Object { Write-Host "  - $_" -ForegroundColor Gray }
    }
} catch {
    Write-Host "⚠️ Could not check installed extensions (VS Code command line not available)" -ForegroundColor Yellow
}

Write-Host "`n🎯 TROUBLESHOOTING STEPS:" -ForegroundColor Cyan
Write-Host "=========================" -ForegroundColor Cyan
Write-Host "If you're still getting 'no suitable targets were found':"
Write-Host "1. ⭐ CLOSE VS Code completely (not just the window)" -ForegroundColor White
Write-Host "2. ⭐ REOPEN VS Code in this folder" -ForegroundColor White
Write-Host "3. ⭐ Wait for all extensions to load" -ForegroundColor White
Write-Host "4. Open src/main.cpp" -ForegroundColor White
Write-Host "5. Press Ctrl+Shift+P" -ForegroundColor White
Write-Host "6. Type: 'PVS-Studio: Analyze Current File'" -ForegroundColor White

Write-Host "`n🔧 If still not working:" -ForegroundColor Yellow
Write-Host "- Try 'PVS-Studio: Analyze Workspace' instead" -ForegroundColor White
Write-Host "- Check VS Code Output panel -> PVS-Studio for error messages" -ForegroundColor White
Write-Host "- Ensure VS Code has finished loading all extensions" -ForegroundColor White

Write-Host "`n✨ Setup verification complete!" -ForegroundColor Green
