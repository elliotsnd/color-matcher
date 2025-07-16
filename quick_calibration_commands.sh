#!/bin/bash
# Quick Matrix Calibration Commands
# UPDATE DEVICE_IP with your actual device IP

DEVICE_IP="192.168.1.100"  # Change this to your device IP
BASE_URL="http://$DEVICE_IP"

echo "🎯 QUICK MATRIX CALIBRATION COMMANDS"
echo "Update DEVICE_IP=$DEVICE_IP with your actual device IP"
echo ""

# Function to get current color
get_color() {
    echo "📖 Current Color Reading:"
    curl -s "$BASE_URL/api/color" | python -m json.tool
    echo ""
}

# Function to update matrix value
update_matrix() {
    local matrix_name=$1
    local index=$2
    local value=$3
    echo "🔧 Updating ${matrix_name}${index} = $value"
    curl -s -X POST "$BASE_URL/api/calibration" -d "${matrix_name}${index}=$value"
    echo ""
}

# Show current reading
echo "📊 Getting current color reading..."
get_color

echo "📋 MANUAL ADJUSTMENT EXAMPLES:"
echo ""
echo "# Get current color:"
echo "curl -s $BASE_URL/api/color | python -m json.tool"
echo ""
echo "# Update bright matrix value (for vivid white):"
echo "curl -s -X POST $BASE_URL/api/calibration -d 'brightMatrix0=0.1054'"
echo ""
echo "# Update dark matrix value (for grey port):"
echo "curl -s -X POST $BASE_URL/api/calibration -d 'darkMatrix0=0.037'"
echo ""
echo "# Multiple parameters at once:"
echo "curl -s -X POST $BASE_URL/api/calibration -d 'brightMatrix0=0.1054&brightMatrix1=-0.017&brightMatrix2=-0.026'"
echo ""

echo "🎯 TARGETS:"
echo "  Vivid White: RGB(247, 248, 244)"
echo "  Grey Port:   RGB(168, 160, 147)"
echo ""

echo "Matrix indices:"
echo "  0,1,2 = First row  (Red coefficients)"
echo "  3,4,5 = Second row (Green coefficients)"
echo "  6,7,8 = Third row  (Blue coefficients)"
