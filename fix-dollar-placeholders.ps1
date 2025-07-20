#!/usr/bin/env pwsh
# Fix all $1 placeholders left behind from sed replacements

$sourceFile = "src/main.cpp"
if (-not (Test-Path $sourceFile)) {
    Write-Error "Source file not found: $sourceFile"
    exit 1
}

Write-Host "🔧 Fixing $1 placeholders in $sourceFile..." -ForegroundColor Green

# Read the entire file
$content = Get-Content $sourceFile -Raw

# Track changes
$changeCount = 0

# Fix all $1 placeholders - they should be removed since they're malformed serial prints
$patterns = @(
    # Remove malformed serial print statements with $1
    'Serial\.print\(\$1, DECIMAL_PRECISION_\d+\);?\s*',
    'String\(\$1, DECIMAL_PRECISION_\d+\)',
    'String\(\$1, \d+\)'
)

foreach ($pattern in $patterns) {
    $regexMatches = [regex]::Matches($content, $pattern)
    if ($regexMatches.Count -gt 0) {
        Write-Host "Removing $($regexMatches.Count) instances of pattern: $pattern" -ForegroundColor Yellow
        $content = $content -replace $pattern, ''
        $changeCount += $regexMatches.Count
    }
}

# Write the file back
Set-Content -Path $sourceFile -Value $content -NoNewline

Write-Host "✅ Fixed $changeCount $1 placeholders in $sourceFile" -ForegroundColor Green

# Verify no $1 remains
$remainingDollarSigns = ($content | Select-String '\$1' -AllMatches).Matches.Count
if ($remainingDollarSigns -gt 0) {
    Write-Warning "⚠️ Still found $remainingDollarSigns instances of $1 in file"
    # Show where they are
    $content | Select-String '\$1' | ForEach-Object {
        Write-Host "Line $($_.LineNumber): $($_.Line.Trim())" -ForegroundColor Red
    }
} else {
    Write-Host "✅ All $1 placeholders successfully removed!" -ForegroundColor Green
}
