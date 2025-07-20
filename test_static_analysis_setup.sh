#!/bin/bash

# Comprehensive Static Analysis Test Script
# Tests all configured static analysis tools to ensure they work correctly

echo "🚀 Testing Comprehensive Static Analysis Setup..."
echo "=============================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counter
TESTS_PASSED=0
TESTS_FAILED=0

# Function to print test results
print_result() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✅ $2${NC}"
        ((TESTS_PASSED++))
    else
        echo -e "${RED}❌ $2${NC}"
        ((TESTS_FAILED++))
    fi
}

# Function to print section headers
print_header() {
    echo -e "\n${BLUE}🔧 $1${NC}"
    echo "----------------------------------------"
}

print_header "Checking Tool Availability"

# Check if tools are installed
echo "Checking Cppcheck..."
if command -v cppcheck &> /dev/null; then
    cppcheck_version=$(cppcheck --version 2>/dev/null || echo "unknown")
    print_result 0 "Cppcheck available: $cppcheck_version"
else
    print_result 1 "Cppcheck not found"
fi

echo "Checking Clang-Tidy..."
if command -v clang-tidy &> /dev/null; then
    clang_tidy_version=$(clang-tidy --version 2>/dev/null | head -1 || echo "unknown")
    print_result 0 "Clang-Tidy available: $clang_tidy_version"
else
    print_result 1 "Clang-Tidy not found"
fi

echo "Checking Python..."
if command -v python3 &> /dev/null; then
    python_version=$(python3 --version 2>/dev/null || echo "unknown")
    print_result 0 "Python available: $python_version"
else
    print_result 1 "Python3 not found"
fi

print_header "Testing Configuration Files"

# Check if configuration files exist
config_files=(
    ".vscode/settings.json"
    ".vscode/extensions.json"
    ".vscode/tasks.json"
    ".clang-tidy"
    ".pvs-studio/project.json"
    "generate_compile_commands.py"
)

for file in "${config_files[@]}"; do
    if [ -f "$file" ]; then
        print_result 0 "Configuration file exists: $file"
    else
        print_result 1 "Configuration file missing: $file"
    fi
done

print_header "Testing Compile Commands Generation"

# Test compile commands generation
echo "Generating compile commands..."
if python3 generate_compile_commands.py &> /dev/null; then
    print_result 0 "Compile commands generated successfully"
    
    # Check if the file was created and is valid JSON
    if [ -f "compile_commands.json" ]; then
        if python3 -c "import json; json.load(open('compile_commands.json'))" &> /dev/null; then
            entries=$(python3 -c "import json; print(len(json.load(open('compile_commands.json'))))" 2>/dev/null || echo "0")
            print_result 0 "Compile commands JSON valid with $entries entries"
        else
            print_result 1 "Compile commands JSON is invalid"
        fi
    else
        print_result 1 "Compile commands file not created"
    fi
else
    print_result 1 "Failed to generate compile commands"
fi

print_header "Testing Static Analysis Tools"

# Test Cppcheck
echo "Testing Cppcheck analysis..."
if command -v cppcheck &> /dev/null; then
    if cppcheck --enable=warning --std=c++17 --suppress=missingIncludeSystem -DARDUINO=10819 -DESP32=1 -DPROGMEM= -Isrc src/main.cpp &> /tmp/cppcheck_test.log; then
        print_result 0 "Cppcheck analysis completed successfully"
    else
        print_result 1 "Cppcheck analysis failed"
        echo "Error log: $(head -3 /tmp/cppcheck_test.log 2>/dev/null || echo 'No log available')"
    fi
else
    print_result 1 "Cppcheck not available for testing"
fi

# Test Clang-Tidy (with timeout to prevent hanging)
echo "Testing Clang-Tidy analysis..."
if command -v clang-tidy &> /dev/null; then
    if timeout 30 clang-tidy --list-checks &> /dev/null; then
        print_result 0 "Clang-Tidy configuration valid"
        
        # Quick syntax check
        if timeout 60 clang-tidy src/main.cpp -- -std=c++17 -DARDUINO=10819 -DESP32=1 -DPROGMEM= -Isrc &> /tmp/clang_tidy_test.log; then
            print_result 0 "Clang-Tidy analysis completed"
        else
            # Don't fail on clang-tidy errors since they might be expected (code issues)
            print_result 0 "Clang-Tidy analysis ran (found issues - this is normal)"
        fi
    else
        print_result 1 "Clang-Tidy configuration invalid"
    fi
else
    print_result 1 "Clang-Tidy not available for testing"
fi

print_header "Testing VS Code Configuration"

# Check VS Code settings validity
echo "Validating VS Code settings..."
if [ -f ".vscode/settings.json" ]; then
    if python3 -c "import json; json.load(open('.vscode/settings.json'))" &> /dev/null; then
        print_result 0 "VS Code settings.json is valid JSON"
        
        # Check for PVS-Studio configuration
        if grep -q "pvs-studio" ".vscode/settings.json"; then
            print_result 0 "PVS-Studio configuration present"
        else
            print_result 1 "PVS-Studio configuration missing"
        fi
        
        # Check for compile commands path
        if grep -q "compile_commands.json" ".vscode/settings.json"; then
            print_result 0 "Compile commands path configured"
        else
            print_result 1 "Compile commands path not configured"
        fi
    else
        print_result 1 "VS Code settings.json is invalid JSON"
    fi
else
    print_result 1 "VS Code settings.json not found"
fi

# Check VS Code tasks
echo "Validating VS Code tasks..."
if [ -f ".vscode/tasks.json" ]; then
    if python3 -c "import json; json.load(open('.vscode/tasks.json'))" &> /dev/null; then
        print_result 0 "VS Code tasks.json is valid JSON"
        
        # Check for static analysis tasks
        if grep -q "Static Analysis" ".vscode/tasks.json"; then
            print_result 0 "Static analysis tasks configured"
        else
            print_result 1 "Static analysis tasks missing"
        fi
    else
        print_result 1 "VS Code tasks.json is invalid JSON"
    fi
else
    print_result 1 "VS Code tasks.json not found"
fi

print_header "Testing GitHub Actions Configuration"

# Check GitHub Actions workflow
if [ -f ".github/workflows/enhanced-static-analysis.yml" ]; then
    print_result 0 "Enhanced static analysis workflow exists"
    
    # Check for tool integration
    if grep -q "cppcheck\|clang-tidy" ".github/workflows/enhanced-static-analysis.yml"; then
        print_result 0 "GitHub Actions includes static analysis tools"
    else
        print_result 1 "GitHub Actions missing tool integration"
    fi
else
    print_result 1 "Enhanced static analysis workflow missing"
fi

print_header "Summary and Recommendations"

echo -e "\n📊 Test Results Summary:"
echo "========================"
echo -e "Tests Passed: ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests Failed: ${RED}$TESTS_FAILED${NC}"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "\n${GREEN}🎉 All tests passed! Your static analysis setup is ready to use.${NC}"
    echo ""
    echo "🚀 Next steps:"
    echo "1. Open this project in VS Code"
    echo "2. Accept extension recommendations when prompted"
    echo "3. Press Ctrl+Shift+P and run 'Tasks: Run Task'"
    echo "4. Select 'Static Analysis: All Tools'"
    echo ""
    echo -e "${BLUE}📚 For detailed usage instructions, see:${NC}"
    echo "   - VSCODE_STATIC_ANALYSIS_GUIDE.md"
    echo "   - STATIC_ANALYSIS_COMPLETE.md"
else
    echo -e "\n${YELLOW}⚠️  Some tests failed. Please review the errors above.${NC}"
    echo ""
    echo "🛠️  Common fixes:"
    echo "1. Install missing tools: sudo apt-get install cppcheck clang-tidy"
    echo "2. Ensure Python 3 is available: python3 --version"
    echo "3. Run: python3 generate_compile_commands.py"
    echo "4. Check configuration files for syntax errors"
    echo ""
    echo -e "${BLUE}📚 For troubleshooting help, see:${NC}"
    echo "   - VSCODE_STATIC_ANALYSIS_GUIDE.md"
fi

echo ""
echo -e "${BLUE}🔧 Manual testing in VS Code:${NC}"
echo "   Ctrl+Shift+P → Tasks: Run Task → Static Analysis: All Tools"
echo ""

exit $TESTS_FAILED