# Advanced Clang-Tidy Warning Fix Script
# Systematically fixes medium-severity warnings in ESP32 C++ code

Write-Host "🔧 Starting systematic fix of medium-severity warnings..." -ForegroundColor Green

# Define the main source file
$sourceFile = "src\main.cpp"

# Backup the original file
$backupFile = "$sourceFile.backup"
Copy-Item $sourceFile $backupFile -Force
Write-Host "✅ Created backup: $backupFile" -ForegroundColor Blue

# Read the file content
$content = Get-Content $sourceFile -Raw

# Count occurrences before fixes
$initialLines = (Get-Content $sourceFile).Count
Write-Host "📊 Initial file has $initialLines lines" -ForegroundColor Cyan

# Track changes made
$changeCount = 0

# Function to apply a replacement and count changes
function Apply-Fix {
    param (
        [ref]$Content,
        [string]$Pattern,
        [string]$Replacement,
        [string]$Description
    )
    
    $beforeCount = ($Content.Value | Select-String $Pattern -AllMatches).Matches.Count
    if ($beforeCount -gt 0) {
        $Content.Value = $Content.Value -replace $Pattern, $Replacement
        $afterCount = ($Content.Value | Select-String $Pattern -AllMatches).Matches.Count
        $fixed = $beforeCount - $afterCount
        if ($fixed -gt 0) {
            Write-Host "  ✓ $Description ($fixed instances)" -ForegroundColor Yellow
            $script:changeCount += $fixed
        }
    }
}

Write-Host "`n🎯 Applying systematic fixes..." -ForegroundColor Green

# Fix HTTP status codes
Apply-Fix ([ref]$content) 'request->send\(404,' 'request->send(HTTP_NOT_FOUND,' 'HTTP 404 status codes'
Apply-Fix ([ref]$content) 'request->send\(400,' 'request->send(HTTP_BAD_REQUEST,' 'HTTP 400 status codes'
Apply-Fix ([ref]$content) 'request->send\(429,' 'request->send(HTTP_TOO_MANY_REQUESTS,' 'HTTP 429 status codes'
Apply-Fix ([ref]$content) '->beginResponse\(200,' '->beginResponse(HTTP_OK,' 'HTTP 200 in beginResponse'

# Fix common 1024 (bytes per KB) occurrences
Apply-Fix ([ref]$content) '([^/])\s*/\s*1024(?!\s*\*|\s*/|MB)' '$1 / BYTES_PER_KB' 'Division by 1024 for KB conversion'
Apply-Fix ([ref]$content) '\*\s*1024(?!\s*\*|\s*/|MB)' '* BYTES_PER_KB' 'Multiplication by 1024 for KB conversion'

# Fix RGB max values
Apply-Fix ([ref]$content) '\*\s*255\.0f' '* RGB_MAX' 'RGB float max value'
Apply-Fix ([ref]$content) '<=\s*255(?![0-9])' '<= RGB_MAX_INT' 'RGB integer max value'
Apply-Fix ([ref]$content) '>\s*255(?![0-9])' '> RGB_MAX_INT' 'RGB integer max comparison'

# Fix percentage calculations
Apply-Fix ([ref]$content) '\*\s*100\s*\)' '* PERCENTAGE_SCALE)' 'Percentage calculations'

# Fix color threshold values
Apply-Fix ([ref]$content) '>\s*200\s*&&' '> COLOR_THRESHOLD_HIGH &&' 'High color threshold'
Apply-Fix ([ref]$content) '<\s*50\s*&&' '< COLOR_THRESHOLD_LOW &&' 'Low color threshold'

# Fix decimal precision values
Apply-Fix ([ref]$content) 'String\([^,]+,\s*6\)' 'String($1, DECIMAL_PRECISION_6)' 'Decimal precision 6'
Apply-Fix ([ref]$content) 'String\([^,]+,\s*10\)' 'String($1, DECIMAL_PRECISION_10)' 'Decimal precision 10'
Apply-Fix ([ref]$content) 'print\([^,]+,\s*6\)' 'print($1, DECIMAL_PRECISION_6)' 'Serial print precision 6'
Apply-Fix ([ref]$content) 'print\([^,]+,\s*10\)' 'print($1, DECIMAL_PRECISION_10)' 'Serial print precision 10'

# Fix large distance and color database thresholds
Apply-Fix ([ref]$content) 'colorCount\s*<=\s*1000' 'colorCount <= LARGE_COLOR_DB_THRESHOLD' 'Large color DB threshold'
Apply-Fix ([ref]$content) 'colorCount\s*>\s*1000' 'colorCount > LARGE_COLOR_DB_THRESHOLD' 'Large color DB threshold comparison'
Apply-Fix ([ref]$content) 'colorCount\s*>\s*10000' 'colorCount > VERY_LARGE_COLOR_DB_THRESHOLD' 'Very large color DB threshold'
Apply-Fix ([ref]$content) 'distance\s*<\s*0\.1f' 'distance < VERY_SMALL_DISTANCE' 'Very small distance threshold'
Apply-Fix ([ref]$content) 'minDistance\s*=\s*999999\.0f' 'minDistance = LARGE_DISTANCE' 'Large distance initialization'

# Fix sensor max value
Apply-Fix ([ref]$content) '/\s*65535\.0f' '/ MAX_SENSOR_VALUE' 'Sensor max value normalization'

# Fix gamma correction
Apply-Fix ([ref]$content) '/\s*2\.2f' '/ GAMMA_CORRECTION' 'Gamma correction factor'

# Fix matrix array size
Apply-Fix ([ref]$content) 'Matrix\[9\]' 'Matrix[MATRIX_SIZE]' 'Matrix array size'
Apply-Fix ([ref]$content) 'float\s+([a-zA-Z_][a-zA-Z0-9_]*)\[9\]' 'float $1[MATRIX_SIZE]' 'Float matrix array size'

# Write the modified content back to the file
Set-Content -Path $sourceFile -Value $content -NoNewline

Write-Host "`n📈 Summary:" -ForegroundColor Green
Write-Host "  Total fixes applied: $changeCount" -ForegroundColor Yellow
$finalLines = (Get-Content $sourceFile).Count
Write-Host "  Final file has $finalLines lines" -ForegroundColor Cyan

if ($changeCount -gt 0) {
    Write-Host "`n🎉 Medium warning fixes completed!" -ForegroundColor Green
    Write-Host "🔄 To restore original file if needed: Copy-Item '$backupFile' '$sourceFile' -Force" -ForegroundColor Blue
} else {
    Write-Host "`n⚠️ No fixes were applied. File may already be optimized." -ForegroundColor Yellow
}

Write-Host "`n🧪 Running test compilation to verify fixes..." -ForegroundColor Green
& pio check --severity=medium --flags "--quiet" | Select-String "Total" | Select-Object -Last 1

Write-Host "`n✅ Fix script completed!" -ForegroundColor Green
