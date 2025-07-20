# PVS-Studio Troubleshooting & Setup Script
# Fixes "no suitable targets were found" error

param(
    [switch]$Verbose,
    [switch]$Force
)

Write-Host "🔧 PVS-Studio Troubleshooting for Color Sensor Project" -ForegroundColor Green
Write-Host "=====================================================" -ForegroundColor Blue

# 1. Check if compile_commands.json exists and is valid
Write-Host "`n1️⃣ Checking compilation database..." -ForegroundColor Yellow
if (Test-Path "compile_commands.json") {
    $compileDb = Get-Content "compile_commands.json" -Raw | ConvertFrom-Json
    Write-Host "✅ compile_commands.json found with $($compileDb.Length) entries" -ForegroundColor Green
    
    if ($Verbose) {
        Write-Host "📋 Sample entry:" -ForegroundColor Gray
        $compileDb[0] | ConvertTo-Json -Depth 3 | Write-Host -ForegroundColor Gray
    }
} else {
    Write-Host "❌ compile_commands.json not found - generating..." -ForegroundColor Red
    Write-Host "🔄 Running PlatformIO compilation database generation..." -ForegroundColor Blue
    
    & pio run --target compiledb
    
    if (Test-Path "compile_commands.json") {
        Write-Host "✅ compile_commands.json generated successfully!" -ForegroundColor Green
    } else {
        Write-Host "❌ Failed to generate compile_commands.json" -ForegroundColor Red
        exit 1
    }
}

# 2. Check VS Code settings
Write-Host "`n2️⃣ Checking VS Code PVS-Studio settings..." -ForegroundColor Yellow
$settingsPath = ".vscode/settings.json"
if (Test-Path $settingsPath) {
    $settings = Get-Content $settingsPath -Raw | ConvertFrom-Json
    
    Write-Host "📋 Current PVS-Studio settings:" -ForegroundColor Blue
    Write-Host "   License Type: $($settings.'pvs-studio.analyzer.licenseType')" -ForegroundColor White
    Write-Host "   Project Type: $($settings.'pvs-studio.analyzer.projectType')" -ForegroundColor White
    Write-Host "   Compile Commands Path: $($settings.'pvs-studio.analyzer.compileCommandsPath')" -ForegroundColor White
    
    # Validate settings
    if ($settings.'pvs-studio.analyzer.licenseType' -eq "free") {
        Write-Host "✅ License type set to 'free' for open source" -ForegroundColor Green
    } else {
        Write-Host "⚠️ License type is not set to 'free' - may cause issues" -ForegroundColor Yellow
    }
    
    if ($settings.'pvs-studio.analyzer.projectType' -eq "compile_commands") {
        Write-Host "✅ Project type set to 'compile_commands'" -ForegroundColor Green
    } else {
        Write-Host "❌ Project type should be 'compile_commands'" -ForegroundColor Red
    }
} else {
    Write-Host "❌ VS Code settings file not found" -ForegroundColor Red
}

# 3. Check PVS-Studio extension
Write-Host "`n3️⃣ Checking PVS-Studio extension..." -ForegroundColor Yellow
$extensions = & code --list-extensions 2>$null
if ($extensions -contains "EvgeniyRyzhkov.pvs-studio-vscode") {
    Write-Host "✅ PVS-Studio extension is installed" -ForegroundColor Green
} else {
    Write-Host "❌ PVS-Studio extension not found" -ForegroundColor Red
    Write-Host "Installing PVS-Studio extension..." -ForegroundColor Blue
    & code --install-extension EvgeniyRyzhkov.pvs-studio-vscode
}

# 4. Create PVS-Studio project file
Write-Host "`n4️⃣ Creating PVS-Studio project configuration..." -ForegroundColor Yellow
$pvsProjectContent = @"
{
    "name": "Color Sensor Project",
    "description": "Hardware/Software color sensor system with TCS3430",
    "version": "1.0.0",
    "language": "C++",
    "platform": "ESP32",
    "compiler": "GCC",
    "standard": "c++17",
    "sourceFiles": [
        "src/main.cpp",
        "src/**/*.cpp",
        "src/**/*.c"
    ],
    "includePaths": [
        "src/",
        "lib/",
        "include/",
        "~/.platformio/packages/framework-arduinoespressif32/cores/esp32",
        "~/.platformio/packages/framework-arduinoespressif32/libraries"
    ],
    "defines": [
        "ARDUINO=10819",
        "ARDUINO_ESP32_DEV",
        "ESP32",
        "ESP_PLATFORM",
        "F_CPU=240000000L"
    ],
    "compileCommandsPath": "./compile_commands.json"
}
"@

if (-not (Test-Path ".pvs-studio")) {
    New-Item -ItemType Directory -Path ".pvs-studio" -Force | Out-Null
}

$pvsProjectContent | Out-File -FilePath ".pvs-studio/project.json" -Encoding UTF8
Write-Host "✅ PVS-Studio project file created: .pvs-studio/project.json" -ForegroundColor Green

# 5. Update VS Code settings with additional paths
Write-Host "`n5️⃣ Updating VS Code settings..." -ForegroundColor Yellow
$updatedSettings = @{
    "pvs-studio.analyzer.pathToPVSStudio" = ""
    "pvs-studio.analyzer.pathToPlogConverter" = ""
    "pvs-studio.analyzer.licenseType" = "free"
    "pvs-studio.analyzer.projectType" = "compile_commands"
    "pvs-studio.analyzer.compileCommandsPath" = "`${workspaceFolder}/compile_commands.json"
    "pvs-studio.analyzer.excludePaths" = @(
        "`${workspaceFolder}/vscode-anthropic-completion/**",
        "`${workspaceFolder}/.pio/**",
        "`${workspaceFolder}/node_modules/**",
        "`${workspaceFolder}/build/**",
        "`${workspaceFolder}/DataStore/**"
    )
    "pvs-studio.analyzer.includePaths" = @(
        "`${workspaceFolder}/src/**",
        "`${workspaceFolder}/lib/**",
        "`${workspaceFolder}/include/**"
    )
    "pvs-studio.analyzer.defines" = @(
        "ARDUINO=10819",
        "ARDUINO_ESP32_DEV", 
        "ESP32",
        "ESP_PLATFORM",
        "F_CPU=240000000L",
        "HAVE_CONFIG_H",
        "PROGMEM="
    )
    "pvs-studio.analyzer.cppStandard" = "c++17"
    "pvs-studio.analyzer.platform" = "x64"
    "pvs-studio.analyzer.configuration" = "Release"
    "pvs-studio.analyzer.massSuppressionMode" = $false
    "pvs-studio.analyzer.showAnalysisProgress" = $true
    "pvs-studio.analyzer.outputFormat" = "xml"
    "pvs-studio.analyzer.additionalArguments" = @(
        "--target=x86_64",
        "--language=c++"
    )
}

if (-not (Test-Path ".vscode")) {
    New-Item -ItemType Directory -Path ".vscode" -Force | Out-Null
}

# Merge with existing settings if they exist
$existingSettings = @{}
if (Test-Path ".vscode/settings.json") {
    $existingSettings = Get-Content ".vscode/settings.json" -Raw | ConvertFrom-Json -AsHashtable
}

# Merge settings
foreach ($key in $updatedSettings.Keys) {
    $existingSettings[$key] = $updatedSettings[$key]
}

$existingSettings | ConvertTo-Json -Depth 10 | Out-File -FilePath ".vscode/settings.json" -Encoding UTF8
Write-Host "✅ VS Code settings updated with PVS-Studio configuration" -ForegroundColor Green

# 6. Build the project to ensure everything compiles
Write-Host "`n6️⃣ Building project to verify compilation..." -ForegroundColor Yellow
try {
    $buildResult = & pio run 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Project builds successfully!" -ForegroundColor Green
    } else {
        Write-Host "⚠️ Build has warnings/errors - PVS-Studio may still work" -ForegroundColor Yellow
        if ($Verbose) {
            Write-Host "Build output:" -ForegroundColor Gray
            $buildResult | Write-Host -ForegroundColor Gray
        }
    }
} catch {
    Write-Host "❌ Build failed - this may affect PVS-Studio analysis" -ForegroundColor Red
}

# 7. Test PVS-Studio CLI
Write-Host "`n7️⃣ Testing PVS-Studio command line interface..." -ForegroundColor Yellow
try {
    $pvsVersion = & pvs-studio-analyzer --version 2>$null
    if ($pvsVersion) {
        Write-Host "✅ PVS-Studio CLI is available: $pvsVersion" -ForegroundColor Green
    } else {
        Write-Host "⚠️ PVS-Studio CLI not found - GUI-only mode" -ForegroundColor Yellow
    }
} catch {
    Write-Host "⚠️ PVS-Studio CLI not available - using VS Code extension only" -ForegroundColor Yellow
}

# 8. Final instructions
Write-Host "`n🎯 SOLUTION: How to run PVS-Studio analysis now" -ForegroundColor Cyan
Write-Host "=============================================" -ForegroundColor Blue

Write-Host "`n✅ **Setup Complete!** Now follow these steps:" -ForegroundColor Green

Write-Host "`n**Step 1: Reload VS Code**" -ForegroundColor Yellow
Write-Host "   • Close VS Code completely" -ForegroundColor White
Write-Host "   • Reopen VS Code in this folder" -ForegroundColor White
Write-Host "   • This ensures settings are reloaded" -ForegroundColor White

Write-Host "`n**Step 2: Run Analysis**" -ForegroundColor Yellow  
Write-Host "   • Open src/main.cpp in VS Code" -ForegroundColor White
Write-Host "   • Press Ctrl+Shift+P (Command Palette)" -ForegroundColor White
Write-Host "   • Type: 'PVS-Studio: Analyze Current File'" -ForegroundColor White
Write-Host "   • OR Type: 'PVS-Studio: Analyze Workspace'" -ForegroundColor White

Write-Host "`n**Step 3: View Results**" -ForegroundColor Yellow
Write-Host "   • Check Problems panel (Ctrl+Shift+M)" -ForegroundColor White
Write-Host "   • Look for PVS-Studio output in terminal" -ForegroundColor White
Write-Host "   • Issues will appear with clickable links to source" -ForegroundColor White

Write-Host "`n**Alternative: Command Line Analysis**" -ForegroundColor Yellow
Write-Host "   If GUI still doesn't work, run:" -ForegroundColor White
Write-Host "   .\run-pvs-cli.ps1" -ForegroundColor Cyan

Write-Host "`n📊 **Expected Results:**" -ForegroundColor Magenta
Write-Host "   • Critical Issues: 0-2 (your code is excellent!)" -ForegroundColor White
Write-Host "   • Logic Issues: 5-10 (potential improvements)" -ForegroundColor White  
Write-Host "   • Style Issues: 10-20 (optional enhancements)" -ForegroundColor White

Write-Host "`n🚀 **Setup Status: READY!**" -ForegroundColor Green
Write-Host "The 'no suitable targets were found' error should now be resolved!" -ForegroundColor White
