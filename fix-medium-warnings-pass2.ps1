# Advanced Clang-Tidy Warning Fix Script - Pass 2
# Focuses on function declarations, static keywords, and remaining patterns

Write-Host "🔧 Starting Pass 2: Function static declarations and remaining fixes..." -ForegroundColor Green

$sourceFile = "src\main.cpp"
$content = Get-Content $sourceFile -Raw
$changeCount = 0

# Function to apply a replacement and count changes
function Apply-Fix {
    param (
        [ref]$Content,
        [string]$Pattern,
        [string]$Replacement,
        [string]$Description
    )
    
    $beforeMatches = [regex]::Matches($Content.Value, $Pattern)
    $beforeCount = $beforeMatches.Count
    if ($beforeCount -gt 0) {
        $Content.Value = [regex]::Replace($Content.Value, $Pattern, $Replacement)
        $afterMatches = [regex]::Matches($Content.Value, $Pattern)
        $afterCount = $afterMatches.Count
        $fixed = $beforeCount - $afterCount
        if ($fixed -gt 0) {
            Write-Host "  ✓ $Description ($fixed instances)" -ForegroundColor Yellow
            $script:changeCount += $fixed
        }
    }
}

Write-Host "`n🎯 Applying Pass 2 fixes..." -ForegroundColor Green

# Fix function definitions that should be static (implementation functions)
$functionPatterns = @(
    @('void (handleRoot|handleCSS|handleJS|handleColorAPI)\(', 'static void $1(', 'Handler function declarations'),
    @('void (loadFallbackColors|loadColorDatabase|cleanupColorDatabase|analyzeSystemPerformance)\(', 'static void $1(', 'Internal utility functions'),
    @('float (calculateColorDistance)\(', 'static float $1(', 'Internal calculation functions'),
    @('String (findClosestDuluxColor)\(', 'static String $1(', 'Internal search functions'),
    @('bool (connectToWiFiOrStartAP)\(', 'static bool $1(', 'Internal setup functions')
)

foreach ($pattern in $functionPatterns) {
    Apply-Fix ([ref]$content) $pattern[0] $pattern[1] $pattern[2]
}

# Fix more HTTP status codes that weren't caught in first pass
Apply-Fix ([ref]$content) '\bsend\(200,' 'send(HTTP_OK,' 'Remaining HTTP 200 codes'

# Fix additional 1024 patterns
Apply-Fix ([ref]$content) '\b1024\s*\*\s*1024\b' 'BYTES_PER_KB * BYTES_PER_KB' 'MB calculations (1024*1024)'

# Fix delay values
Apply-Fix ([ref]$content) 'delay\(500\)' 'delay(500)' 'Standard delays (keeping as-is for now)'
Apply-Fix ([ref]$content) 'delay\(1000\)' 'delay(1000)' 'Standard delays (keeping as-is for now)'

# Fix more IR compensation values
Apply-Fix ([ref]$content) '<=\s*2\.0\b' '<= MAX_IR_COMPENSATION' 'IR compensation max value'

# Fix more integration time values
Apply-Fix ([ref]$content) '<=\s*255(?!\.)' '<= MAX_INTEGRATION_TIME' 'Integration time max value'

# Fix sample limits
Apply-Fix ([ref]$content) '<=\s*10\b(?!\.)' '<= MAX_COLOR_SAMPLES' 'Color samples max value'
Apply-Fix ([ref]$content) '<=\s*50\b(?!\.)' '<= MAX_SAMPLE_DELAY' 'Sample delay max value'

# Fix additional percentage calculations
Apply-Fix ([ref]$content) '/\s*100\s*\)' '/ PERCENTAGE_SCALE)' 'Additional percentage calculations'

# Fix brightness calculations with division by 3
Apply-Fix ([ref]$content) '255\s*/\s*3' 'RGB_MAX_INT / 3' 'RGB brightness division'

# Fix UMS3 pixel brightness
Apply-Fix ([ref]$content) 'setPixelBrightness\(255\s*/\s*3\)' 'setPixelBrightness(RGB_MAX_INT / 3)' 'UMS3 pixel brightness'

# Write the modified content back to the file
Set-Content -Path $sourceFile -Value $content -NoNewline

Write-Host "`n📈 Pass 2 Summary:" -ForegroundColor Green
Write-Host "  Additional fixes applied: $changeCount" -ForegroundColor Yellow

if ($changeCount -gt 0) {
    Write-Host "`n🎉 Pass 2 completed!" -ForegroundColor Green
} else {
    Write-Host "`n⚠️ No additional fixes were applied." -ForegroundColor Yellow
}

Write-Host "`n🧪 Running test compilation after Pass 2..." -ForegroundColor Green
& pio check --severity=medium --flags "--quiet" | Select-String "Total" | Select-Object -Last 1

Write-Host "`n✅ Pass 2 fix script completed!" -ForegroundColor Green
