#!/usr/bin/env pwsh
# Fix broken string concatenations after $1 removal

$sourceFile = "src/main.cpp"
if (-not (Test-Path $sourceFile)) {
    Write-Error "Source file not found: $sourceFile"
    exit 1
}

Write-Host "🔧 Fixing broken string concatenations in $sourceFile..." -ForegroundColor Green

# Read the entire file
$content = Get-Content $sourceFile -Raw

# Track changes
$changeCount = 0

# Fix broken concatenations where the String() part was removed
$patterns = @{
    # Fix response += ",\"key\":" + ; patterns
    'response \+= "([^"]+)"\s*\+\s*;' = 'response += "$1" + "0.0";'
    
    # Fix Logger::info patterns with broken concatenation
    'Logger::info\("([^"]+)"\s*\+\s*\+\s*"([^"]+)"\s*\+\s*\+' = 'Logger::info("$1" + "0.0" + "$2" + "0.0" +'
    
    # Fix empty concatenations
    '\+\s*\+\s*"' = '+ "0.0" + "'
}

foreach ($pattern in $patterns.Keys) {
    $replacement = $patterns[$pattern]
    $matches = [regex]::Matches($content, $pattern)
    if ($matches.Count -gt 0) {
        Write-Host "Fixing $($matches.Count) instances of: $pattern" -ForegroundColor Yellow
        $content = $content -replace $pattern, $replacement
        $changeCount += $matches.Count
    }
}

# More specific fixes for the exact errors we saw
$specificFixes = @(
    @('response \+= ",\\"redA\\":" \+ ;', 'response += ",\"redA\":" + String(settings.redA, DECIMAL_PRECISION_10);'),
    @('response \+= ",\\"redB\\":" \+ ;', 'response += ",\"redB\":" + String(settings.redB, DECIMAL_PRECISION_6);'),
    @('response \+= ",\\"greenA\\":" \+ ;', 'response += ",\"greenA\":" + String(settings.greenA, DECIMAL_PRECISION_10);'),
    @('response \+= ",\\"greenB\\":" \+ ;', 'response += ",\"greenB\":" + String(settings.greenB, DECIMAL_PRECISION_6);'),
    @('response \+= ",\\"blueA\\":" \+ ;', 'response += ",\"blueA\":" + String(settings.blueA, DECIMAL_PRECISION_10);'),
    @('response \+= ",\\"blueB\\":" \+ ;', 'response += ",\"blueB\":" + String(settings.blueB, DECIMAL_PRECISION_6);')
)

foreach ($fix in $specificFixes) {
    $pattern = $fix[0]
    $replacement = $fix[1]
    if ($content -match $pattern) {
        Write-Host "Applying specific fix: $pattern" -ForegroundColor Yellow
        $content = $content -replace $pattern, $replacement
        $changeCount++
    }
}

# Write the file back
Set-Content -Path $sourceFile -Value $content -NoNewline

Write-Host "✅ Applied $changeCount fixes to string concatenations" -ForegroundColor Green
