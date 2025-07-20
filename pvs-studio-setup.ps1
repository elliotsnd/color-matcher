# PVS-Studio Setup and Analysis Script for Color Sensor Project
# This script sets up PVS-Studio configuration and runs analysis

Write-Host "Setting up PVS-Studio for Color Sensor Project..." -ForegroundColor Green

# Check if PVS-Studio extension is available
$pvsStudioPath = Get-Command "PVS-Studio" -ErrorAction SilentlyContinue
if (-not $pvsStudioPath) {
    Write-Host "PVS-Studio command line tool not found. Please ensure PVS-Studio is installed." -ForegroundColor Yellow
    Write-Host "You can download it from: https://pvs-studio.com/en/pvs-studio/download/" -ForegroundColor Yellow
}

# Create PVS-Studio configuration file
Write-Host "Creating PVS-Studio configuration..." -ForegroundColor Blue

$pvsConfig = @"
<?xml version="1.0" encoding="utf-8"?>
<PVS-Studio>
  <Settings>
    <!-- Analysis settings -->
    <Platform>x64</Platform>
    <Configuration>Release</Configuration>
    
    <!-- Include paths for Arduino/ESP32 -->
    <IncludePaths>
      <Path>$(USERPROFILE)\.platformio\packages\framework-arduinoespressif32\cores\esp32</Path>
      <Path>$(USERPROFILE)\.platformio\packages\framework-arduinoespressif32\libraries</Path>
      <Path>$(USERPROFILE)\.platformio\packages\toolchain-xtensa-esp32\xtensa-esp32-elf\include</Path>
      <Path>lib</Path>
      <Path>src</Path>
      <Path>include</Path>
    </IncludePaths>
    
    <!-- Preprocessor definitions for ESP32 -->
    <PreprocessorDefinitions>
      <Definition>ARDUINO=10819</Definition>
      <Definition>ARDUINO_ESP32_DEV</Definition>
      <Definition>ESP32</Definition>
      <Definition>ESP_PLATFORM</Definition>
      <Definition>F_CPU=240000000L</Definition>
      <Definition>HAVE_CONFIG_H</Definition>
      <Definition>MBEDTLS_CONFIG_FILE="mbedtls/esp_config.h"</Definition>
    </PreprocessorDefinitions>
    
    <!-- Analysis rules -->
    <EnabledWarnings>
      <Warning>V001</Warning> <!-- Suspicious assignment -->
      <Warning>V002</Warning> <!-- CWE-571: Expression is always true/false -->
      <Warning>V003</Warning> <!-- Uninitialized variable -->
      <Warning>V004</Warning> <!-- Array overrun -->
      <Warning>V005</Warning> <!-- Memory leak -->
      <Warning>V006</Warning> <!-- Null pointer dereference -->
      <Warning>V501</Warning> <!-- Identical sub-expressions -->
      <Warning>V502</Warning> <!-- Suspicious use of comparison operator -->
      <Warning>V503</Warning> <!-- Suspicious declaration -->
      <Warning>V504</Warning> <!-- Suspicious use of bitwise operation -->
      <Warning>V505</Warning> <!-- Suspicious assignment -->
      <Warning>V506</Warning> <!-- Suspicious increment/decrement -->
      <Warning>V507</Warning> <!-- Suspicious pointer arithmetic -->
      <Warning>V508</Warning> <!-- Variable used after being moved -->
      <Warning>V509</Warning> <!-- Suspicious conditional expression -->
      <Warning>V510</Warning> <!-- Suspicious use of comma operator -->
      <Warning>V511</Warning> <!-- Expression under 'if' is always true/false -->
      <Warning>V512</Warning> <!-- Call to function may lead to buffer overflow -->
      <Warning>V513</Warning> <!-- Suspicious use of logical operation -->
      <Warning>V514</Warning> <!-- Unsafe use of 'goto' statement -->
      <Warning>V515</Warning> <!-- Expression under 'sizeof' never changes -->
      <Warning>V516</Warning> <!-- Function parameter is not used -->
      <Warning>V517</Warning> <!-- Use of 'goto' to skip variable initialization -->
      <Warning>V518</Warning> <!-- Malloc/free mismatch -->
      <Warning>V519</Warning> <!-- Variable is assigned but never used -->
      <Warning>V520</Warning> <!-- Suspicious use of comma in array index -->
      <Warning>V521</Warning> <!-- Expression is always true -->
      <Warning>V522</Warning> <!-- Null pointer dereference -->
      <Warning>V523</Warning> <!-- Expression is always false -->
      <Warning>V524</Warning> <!-- Suspicious use of logical operation -->
      <Warning>V525</Warning> <!-- Buffer overflow -->
      <Warning>V526</Warning> <!-- Variable is used uninitialized -->
      <Warning>V527</Warning> <!-- Unreachable code -->
      <Warning>V528</Warning> <!-- Variable is assigned but never used -->
      <Warning>V529</Warning> <!-- Odd expression -->
      <Warning>V530</Warning> <!-- Return value is not used -->
      <Warning>V531</Warning> <!-- Array overrun -->
      <Warning>V532</Warning> <!-- Consider inspecting the statement -->
      <Warning>V533</Warning> <!-- Two or more case labels -->
      <Warning>V534</Warning> <!-- Dangerous use of 'strlen' -->
      <Warning>V535</Warning> <!-- Variable is used after being freed -->
      <Warning>V536</Warning> <!-- Dangerous cast -->
      <Warning>V537</Warning> <!-- Consider checking format string -->
      <Warning>V538</Warning> <!-- Suspicious use of conditional expression -->
      <Warning>V539</Warning> <!-- Suspicious use of the comma operator -->
      <Warning>V540</Warning> <!-- Member of a class is not initialized -->
      <Warning>V541</Warning> <!-- Dangerous use of 'goto' -->
      <Warning>V542</Warning> <!-- Consider inspecting function call -->
      <Warning>V543</Warning> <!-- Suspicious use of comma operator -->
      <Warning>V544</Warning> <!-- Suspicious use of conditional expression -->
      <Warning>V545</Warning> <!-- Suspicious mixing of integer types -->
      <Warning>V546</Warning> <!-- Member function should be declared const -->
      <Warning>V547</Warning> <!-- Expression is always true/false -->
      <Warning>V548</Warning> <!-- Consider inspecting the function call -->
      <Warning>V549</Warning> <!-- Variable is assigned but never used -->
      <Warning>V550</Warning> <!-- Expression is always true -->
      <Warning>V551</Warning> <!-- Suspicious use of comma operator -->
      <Warning>V552</Warning> <!-- Review array bounds -->
      <Warning>V553</Warning> <!-- Variable is assigned but never used -->
      <Warning>V554</Warning> <!-- Incorrect use of auto_ptr -->
      <Warning>V555</Warning> <!-- Dangerous cast -->
      <Warning>V556</Warning> <!-- Expression will always be true/false -->
      <Warning>V557</Warning> <!-- Array overrun -->
      <Warning>V558</Warning> <!-- Function returns reference to local object -->
      <Warning>V559</Warning> <!-- Suspicious assignment -->
      <Warning>V560</Warning> <!-- Part of conditional expression is always true/false -->
      <Warning>V561</Warning> <!-- Expression is always true -->
      <Warning>V562</Warning> <!-- Odd expression -->
      <Warning>V563</Warning> <!-- Variable is assigned but never used -->
      <Warning>V564</Warning> <!-- Expression is always true -->
      <Warning>V565</Warning> <!-- Empty exception handler -->
      <Warning>V566</Warning> <!-- Unreachable code -->
      <Warning>V567</Warning> <!-- Undefined behavior -->
      <Warning>V568</Warning> <!-- Suspicious assignment -->
      <Warning>V569</Warning> <!-- Expression is always true -->
      <Warning>V570</Warning> <!-- Expression is always false -->
      <Warning>V571</Warning> <!-- Recurring check -->
      <Warning>V572</Warning> <!-- Expression is always true/false -->
      <Warning>V573</Warning> <!-- Uninitialized variable -->
      <Warning>V574</Warning> <!-- Dangerous cast -->
      <Warning>V575</Warning> <!-- Function returns reference to local object -->
      <Warning>V576</Warning> <!-- Incorrect use of 'continue' -->
      <Warning>V577</Warning> <!-- Potential null pointer dereference -->
      <Warning>V578</Warning> <!-- Function returns reference to local variable -->
      <Warning>V579</Warning> <!-- Suspicious assignment -->
      <Warning>V580</Warning> <!-- Member function can be const -->
      <Warning>V581</Warning> <!-- Self-assignment -->
      <Warning>V582</Warning> <!-- Unsafe use of 'goto' -->
      <Warning>V583</Warning> <!-- Function is declared but never defined -->
      <Warning>V584</Warning> <!-- Consider inspecting format string -->
      <Warning>V585</Warning> <!-- Consider inspecting the function call -->
      <Warning>V586</Warning> <!-- Odd expression -->
      <Warning>V587</Warning> <!-- Expression is always true/false -->
      <Warning>V588</Warning> <!-- Variable is assigned but never used -->
      <Warning>V589</Warning> <!-- Expression is always true -->
      <Warning>V590</Warning> <!-- Consider inspecting this expression -->
      <Warning>V591</Warning> <!-- Non-void function should return a value -->
      <Warning>V592</Warning> <!-- Expression is always true/false -->
      <Warning>V593</Warning> <!-- Consider checking format string -->
      <Warning>V594</Warning> <!-- Suspicious expression -->
      <Warning>V595</Warning> <!-- Pointer is used after memory was freed -->
      <Warning>V596</Warning> <!-- Integer division -->
      <Warning>V597</Warning> <!-- Compiler eliminated memset call -->
      <Warning>V598</Warning> <!-- Suspicious expression -->
      <Warning>V599</Warning> <!-- Consider inspecting the function call -->
      <Warning>V600</Warning> <!-- Consider inspecting the condition -->
      <Warning>V601</Warning> <!-- Suspicious expression -->
      <Warning>V602</Warning> <!-- Consider inspecting format string -->
      <Warning>V603</Warning> <!-- Object was created but is not being used -->
      <Warning>V604</Warning> <!-- Dangerous cast -->
      <Warning>V605</Warning> <!-- Consider inspecting the expression -->
      <Warning>V606</Warning> <!-- Dangerous cast -->
      <Warning>V607</Warning> <!-- Ownerless expression -->
      <Warning>V608</Warning> <!-- Dangerous cast -->
      <Warning>V609</Warning> <!-- Divide by zero -->
      <Warning>V610</Warning> <!-- Undefined behavior -->
      <Warning>V611</Warning> <!-- Memory leak -->
      <Warning>V612</Warning> <!-- Suspicious expression -->
      <Warning>V613</Warning> <!-- Potential null pointer dereference -->
      <Warning>V614</Warning> <!-- Suspicious assignment -->
      <Warning>V615</Warning> <!-- Consider inspecting the function call -->
      <Warning>V616</Warning> <!-- Control flow will never pass -->
      <Warning>V617</Warning> <!-- Consider inspecting the condition -->
      <Warning>V618</Warning> <!-- Memory allocated but never used -->
      <Warning>V619</Warning> <!-- Array overrun -->
      <Warning>V620</Warning> <!-- Suspicious expression -->
      <Warning>V621</Warning> <!-- Consider inspecting the function call -->
      <Warning>V622</Warning> <!-- Suspicious expression -->
      <Warning>V623</Warning> <!-- Consider inspecting format string -->
      <Warning>V624</Warning> <!-- Memory leak -->
      <Warning>V625</Warning> <!-- Consider inspecting format string -->
      <Warning>V626</Warning> <!-- Suspicious expression -->
      <Warning>V627</Warning> <!-- Consider inspecting the function call -->
      <Warning>V628</Warning> <!-- Suspicious expression -->
      <Warning>V629</Warning> <!-- Consider inspecting format string -->
      <Warning>V630</Warning> <!-- Function is declared but never called -->
      <Warning>V631</Warning> <!-- Consider inspecting the condition -->
      <Warning>V632</Warning> <!-- Consider inspecting the function call -->
      <Warning>V633</Warning> <!-- Suspicious expression -->
      <Warning>V634</Warning> <!-- Priority of operation is unclear -->
      <Warning>V635</Warning> <!-- Consider inspecting the function call -->
      <Warning>V636</Warning> <!-- Suspicious expression -->
      <Warning>V637</Warning> <!-- Two opposite conditions -->
      <Warning>V638</Warning> <!-- Suspicious expression -->
      <Warning>V639</Warning> <!-- Consider inspecting the function call -->
      <Warning>V640</Warning> <!-- Suspicious expression -->
      <Warning>V641</Warning> <!-- Suspicious expression -->
      <Warning>V642</Warning> <!-- Suspicious expression -->
      <Warning>V643</Warning> <!-- Suspicious expression -->
      <Warning>V644</Warning> <!-- Suspicious expression -->
      <Warning>V645</Warning> <!-- Suspicious expression -->
      <Warning>V646</Warning> <!-- Consider inspecting the function call -->
      <Warning>V647</Warning> <!-- Suspicious expression -->
      <Warning>V648</Warning> <!-- Priority of operation is unclear -->
      <Warning>V649</Warning> <!-- There are two 'if' statements with identical condition -->
      <Warning>V650</Warning> <!-- Type casting operation is suspicious -->
      <Warning>V651</Warning> <!-- Suspicious expression -->
      <Warning>V652</Warning> <!-- Consider inspecting the function call -->
      <Warning>V653</Warning> <!-- Expression is always true/false -->
      <Warning>V654</Warning> <!-- Consider inspecting the function call -->
      <Warning>V655</Warning> <!-- Suspicious expression -->
      <Warning>V656</Warning> <!-- Variables are initialized through the call -->
      <Warning>V657</Warning> <!-- Suspicious expression -->
      <Warning>V658</Warning> <!-- Suspicious expression -->
      <Warning>V659</Warning> <!-- Suspicious expression -->
      <Warning>V660</Warning> <!-- Suspicious expression -->
      <Warning>V661</Warning> <!-- Suspicious expression -->
      <Warning>V662</Warning> <!-- Consider inspecting the function call -->
      <Warning>V663</Warning> <!-- Infinite loop -->
      <Warning>V664</Warning> <!-- Variable is not used after assignment -->
      <Warning>V665</Warning> <!-- Suspicious expression -->
      <Warning>V666</Warning> <!-- Consider inspecting the condition -->
      <Warning>V667</Warning> <!-- Suspicious expression -->
      <Warning>V668</Warning> <!-- There is no sense in testing value -->
      <Warning>V669</Warning> <!-- Suspicious expression -->
      <Warning>V670</Warning> <!-- Uninitialized variable -->
      <Warning>V671</Warning> <!-- Suspicious expression -->
      <Warning>V672</Warning> <!-- Suspicious expression -->
      <Warning>V673</Warning> <!-- More than N bits are required -->
      <Warning>V674</Warning> <!-- Suspicious expression -->
      <Warning>V675</Warning> <!-- Suspicious expression -->
      <Warning>V676</Warning> <!-- Suspicious expression -->
      <Warning>V677</Warning> <!-- Custom declaration of a function -->
      <Warning>V678</Warning> <!-- Suspicious expression -->
      <Warning>V679</Warning> <!-- Suspicious expression -->
      <Warning>V680</Warning> <!-- Suspicious expression -->
      <Warning>V681</Warning> <!-- Suspicious expression -->
      <Warning>V682</Warning> <!-- Buffer underrun -->
      <Warning>V683</Warning> <!-- Operation is redundant -->
      <Warning>V684</Warning> <!-- Suspicious expression -->
      <Warning>V685</Warning> <!-- Consider inspecting the condition -->
      <Warning>V686</Warning> <!-- Suspicious expression -->
      <Warning>V687</Warning> <!-- Suspicious expression -->
      <Warning>V688</Warning> <!-- Suspicious expression -->
      <Warning>V689</Warning> <!-- Suspicious expression -->
      <Warning>V690</Warning> <!-- Copy constructor is declared -->
      <Warning>V691</Warning> <!-- Initialization of a variable -->
      <Warning>V692</Warning> <!-- Suspicious expression -->
      <Warning>V693</Warning> <!-- Consider inspecting the function call -->
      <Warning>V694</Warning> <!-- Suspicious expression -->
      <Warning>V695</Warning> <!-- Range-based for loop variable -->
      <Warning>V696</Warning> <!-- Suspicious expression -->
      <Warning>V697</Warning> <!-- Multiple index variables -->
      <Warning>V698</Warning> <!-- Suspicious expression -->
      <Warning>V699</Warning> <!-- Consider inspecting the function call -->
      <Warning>V700</Warning> <!-- Expression is always true/false -->
      <Warning>V701</Warning> <!-- realloc() possible leak -->
      <Warning>V702</Warning> <!-- Classes should not be compared -->
      <Warning>V703</Warning> <!-- Suspicious expression -->
      <Warning>V704</Warning> <!-- Suspicious expression -->
      <Warning>V705</Warning> <!-- Comma operator is used -->
      <Warning>V706</Warning> <!-- Suspicious expression -->
      <Warning>V707</Warning> <!-- Suspicious expression -->
      <Warning>V708</Warning> <!-- Suspicious expression -->
      <Warning>V709</Warning> <!-- Suspicious expression -->
      <Warning>V710</Warning> <!-- Suspicious expression -->
      <Warning>V711</Warning> <!-- Suspicious expression -->
      <Warning>V712</Warning> <!-- Suspicious expression -->
      <Warning>V713</Warning> <!-- Suspicious expression -->
      <Warning>V714</Warning> <!-- Suspicious expression -->
      <Warning>V715</Warning> <!-- Suspicious expression -->
      <Warning>V716</Warning> <!-- Suspicious expression -->
      <Warning>V717</Warning> <!-- Suspicious expression -->
      <Warning>V718</Warning> <!-- Suspicious expression -->
      <Warning>V719</Warning> <!-- Suspicious expression -->
      <Warning>V720</Warning> <!-- Suspicious expression -->
      <Warning>V721</Warning> <!-- Suspicious expression -->
      <Warning>V722</Warning> <!-- Suspicious expression -->
      <Warning>V723</Warning> <!-- Suspicious expression -->
      <Warning>V724</Warning> <!-- Function returns address -->
      <Warning>V725</Warning> <!-- Suspicious expression -->
      <Warning>V726</Warning> <!-- Suspicious expression -->
      <Warning>V727</Warning> <!-- Suspicious expression -->
      <Warning>V728</Warning> <!-- Suspicious expression -->
      <Warning>V729</Warning> <!-- Suspicious expression -->
      <Warning>V730</Warning> <!-- Suspicious expression -->
      <Warning>V731</Warning> <!-- Suspicious expression -->
      <Warning>V732</Warning> <!-- Suspicious expression -->
      <Warning>V733</Warning> <!-- Suspicious expression -->
      <Warning>V734</Warning> <!-- Suspicious expression -->
      <Warning>V735</Warning> <!-- Suspicious expression -->
      <Warning>V736</Warning> <!-- Suspicious expression -->
      <Warning>V737</Warning> <!-- Suspicious expression -->
      <Warning>V738</Warning> <!-- Suspicious expression -->
      <Warning>V739</Warning> <!-- Expression is always true/false -->
      <Warning>V740</Warning> <!-- Suspicious expression -->
      <Warning>V741</Warning> <!-- Suspicious expression -->
      <Warning>V742</Warning> <!-- Suspicious expression -->
      <Warning>V743</Warning> <!-- Suspicious expression -->
      <Warning>V744</Warning> <!-- Suspicious expression -->
      <Warning>V745</Warning> <!-- Suspicious expression -->
      <Warning>V746</Warning> <!-- Suspicious expression -->
      <Warning>V747</Warning> <!-- Suspicious expression -->
      <Warning>V748</Warning> <!-- Suspicious expression -->
      <Warning>V749</Warning> <!-- Suspicious expression -->
      <Warning>V750</Warning> <!-- Suspicious expression -->
      <Warning>V751</Warning> <!-- Suspicious expression -->
      <Warning>V752</Warning> <!-- Suspicious expression -->
      <Warning>V753</Warning> <!-- Suspicious expression -->
      <Warning>V754</Warning> <!-- Suspicious expression -->
      <Warning>V755</Warning> <!-- Suspicious expression -->
      <Warning>V756</Warning> <!-- Suspicious expression -->
      <Warning>V757</Warning> <!-- Suspicious expression -->
      <Warning>V758</Warning> <!-- Suspicious expression -->
      <Warning>V759</Warning> <!-- Suspicious expression -->
      <Warning>V760</Warning> <!-- Suspicious expression -->
      <Warning>V761</Warning> <!-- Suspicious expression -->
      <Warning>V762</Warning> <!-- Suspicious expression -->
      <Warning>V763</Warning> <!-- Suspicious expression -->
      <Warning>V764</Warning> <!-- Suspicious expression -->
      <Warning>V765</Warning> <!-- Suspicious expression -->
      <Warning>V766</Warning> <!-- Suspicious expression -->
      <Warning>V767</Warning> <!-- Suspicious expression -->
      <Warning>V768</Warning> <!-- Suspicious expression -->
      <Warning>V769</Warning> <!-- Suspicious expression -->
      <Warning>V770</Warning> <!-- Suspicious expression -->
      <Warning>V771</Warning> <!-- Suspicious expression -->
      <Warning>V772</Warning> <!-- Suspicious expression -->
      <Warning>V773</Warning> <!-- Suspicious expression -->
      <Warning>V774</Warning> <!-- Suspicious expression -->
      <Warning>V775</Warning> <!-- Suspicious expression -->
      <Warning>V776</Warning> <!-- Suspicious expression -->
      <Warning>V777</Warning> <!-- Suspicious expression -->
      <Warning>V778</Warning> <!-- Suspicious expression -->
      <Warning>V779</Warning> <!-- Suspicious expression -->
      <Warning>V780</Warning> <!-- Suspicious expression -->
      <Warning>V781</Warning> <!-- Suspicious expression -->
      <Warning>V782</Warning> <!-- Suspicious expression -->
      <Warning>V783</Warning> <!-- Suspicious expression -->
      <Warning>V784</Warning> <!-- Suspicious expression -->
      <Warning>V785</Warning> <!-- Suspicious expression -->
      <Warning>V786</Warning> <!-- Suspicious expression -->
      <Warning>V787</Warning> <!-- Suspicious expression -->
      <Warning>V788</Warning> <!-- Suspicious expression -->
      <Warning>V789</Warning> <!-- Suspicious expression -->
      <Warning>V790</Warning> <!-- Suspicious expression -->
      <Warning>V791</Warning> <!-- Suspicious expression -->
      <Warning>V792</Warning> <!-- Suspicious expression -->
      <Warning>V793</Warning> <!-- Suspicious expression -->
      <Warning>V794</Warning> <!-- Suspicious expression -->
      <Warning>V795</Warning> <!-- Suspicious expression -->
      <Warning>V796</Warning> <!-- Suspicious expression -->
      <Warning>V797</Warning> <!-- Suspicious expression -->
      <Warning>V798</Warning> <!-- Suspicious expression -->
      <Warning>V799</Warning> <!-- Suspicious expression -->
    </EnabledWarnings>
    
    <!-- Output settings -->
    <OutputFormat>xml</OutputFormat>
    <OutputFile>pvs-studio-report.xml</OutputFile>
    
    <!-- Exclude certain files if needed -->
    <ExcludePaths>
      <Path>vscode-anthropic-completion</Path>
      <Path>.pio</Path>
      <Path>node_modules</Path>
      <Path>build</Path>
    </ExcludePaths>
    
  </Settings>
</PVS-Studio>
"@

$pvsConfig | Out-File -FilePath "pvs-studio.cfg" -Encoding UTF8

Write-Host "✓ Created pvs-studio.cfg configuration file" -ForegroundColor Green

# Create a simple analysis batch file for command line usage
$batchContent = @"
@echo off
echo Starting PVS-Studio analysis for Color Sensor Project...

REM Build compile_commands.json using PlatformIO
echo Building compile commands database...
pio run -t compiledb

REM Run PVS-Studio analysis
echo Running PVS-Studio analysis...
PVS-Studio.exe --cfg "pvs-studio.cfg" --source-file compile_commands.json --output-file pvs-studio-report.xml

REM Convert to readable format
echo Converting report to readable format...
PlogConverter.exe -a "GA:1,2,3;64:1,2,3;OP:1,2,3" -t fullhtml -o pvs-studio-report.html pvs-studio-report.xml

echo Analysis complete! Check pvs-studio-report.html for results.
pause
"@

$batchContent | Out-File -FilePath "run-pvs-analysis.bat" -Encoding ASCII

Write-Host "✓ Created run-pvs-analysis.bat script" -ForegroundColor Green

# Create PowerShell version as well
$psAnalysisScript = @"
# PVS-Studio Analysis Script
Write-Host "Starting PVS-Studio analysis for Color Sensor Project..." -ForegroundColor Green

# Check if PlatformIO is available
if (Get-Command "pio" -ErrorAction SilentlyContinue) {
    Write-Host "Building compile commands database..." -ForegroundColor Blue
    & pio run -t compiledb
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Failed to generate compile commands. Please check PlatformIO setup." -ForegroundColor Red
        exit 1
    }
} else {
    Write-Host "PlatformIO not found. Please install PlatformIO first." -ForegroundColor Red
    exit 1
}

# Check if compile_commands.json exists
if (-not (Test-Path "compile_commands.json")) {
    Write-Host "compile_commands.json not found. Analysis cannot proceed." -ForegroundColor Red
    exit 1
}

# Check if PVS-Studio is available
if (Get-Command "PVS-Studio.exe" -ErrorAction SilentlyContinue) {
    Write-Host "Running PVS-Studio analysis..." -ForegroundColor Blue
    & PVS-Studio.exe --cfg "pvs-studio.cfg" --source-file compile_commands.json --output-file pvs-studio-report.xml
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "PVS-Studio analysis failed." -ForegroundColor Red
        exit 1
    }
    
    Write-Host "✓ Analysis complete! Report saved to pvs-studio-report.xml" -ForegroundColor Green
} else {
    Write-Host "PVS-Studio command line tool not found." -ForegroundColor Red
    Write-Host "Please install PVS-Studio from: https://pvs-studio.com/en/pvs-studio/download/" -ForegroundColor Yellow
    exit 1
}

# Convert to HTML if PlogConverter is available
if (Get-Command "PlogConverter.exe" -ErrorAction SilentlyContinue) {
    Write-Host "Converting report to HTML format..." -ForegroundColor Blue
    & PlogConverter.exe -a "GA:1,2,3;64:1,2,3;OP:1,2,3" -t fullhtml -o pvs-studio-report.html pvs-studio-report.xml
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✓ HTML report generated: pvs-studio-report.html" -ForegroundColor Green
        
        # Open the report in default browser
        if (Test-Path "pvs-studio-report.html") {
            Write-Host "Opening report in browser..." -ForegroundColor Blue
            Start-Process "pvs-studio-report.html"
        }
    } else {
        Write-Host "Failed to convert report to HTML." -ForegroundColor Yellow
    }
} else {
    Write-Host "PlogConverter not found. XML report available at pvs-studio-report.xml" -ForegroundColor Yellow
}

Write-Host "Analysis complete!" -ForegroundColor Green
"@

$psAnalysisScript | Out-File -FilePath "run-pvs-analysis.ps1" -Encoding UTF8

Write-Host "✓ Created run-pvs-analysis.ps1 script" -ForegroundColor Green

Write-Host ""
Write-Host "Setup complete! You can now analyze your project using:" -ForegroundColor Cyan
Write-Host "  1. VS Code Extension: Use Ctrl+Shift+P and search for 'PVS-Studio'" -ForegroundColor White
Write-Host "  2. Command Line: Run 'run-pvs-analysis.ps1' or 'run-pvs-analysis.bat'" -ForegroundColor White
Write-Host "  3. Manual: Use the pvs-studio.cfg configuration file" -ForegroundColor White
Write-Host ""
Write-Host "Note: Make sure PVS-Studio command line tools are installed for full functionality." -ForegroundColor Yellow
