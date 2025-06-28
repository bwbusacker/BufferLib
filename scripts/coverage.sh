#!/bin/bash

# Code Coverage Report Generator for BufferLib
# Uses lcov to generate HTML coverage reports

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_DIR="build-coverage"
COVERAGE_DIR="coverage"
LCOV_OUTPUT="coverage.info"

echo -e "${BLUE}BufferLib Code Coverage Report Generator${NC}"
echo "========================================"

# Check if lcov is available
if ! command -v lcov &> /dev/null; then
    echo -e "${RED}Error: lcov is not installed${NC}"
    echo "Install with:"
    echo "  Ubuntu/Debian: sudo apt-get install lcov"
    echo "  macOS: brew install lcov"
    exit 1
fi

# Check if genhtml is available
if ! command -v genhtml &> /dev/null; then
    echo -e "${RED}Error: genhtml is not installed (should come with lcov)${NC}"
    exit 1
fi

echo -e "${YELLOW}Step 1: Clean previous coverage data${NC}"
rm -rf ${BUILD_DIR}
rm -rf ${COVERAGE_DIR}
mkdir -p ${COVERAGE_DIR}

echo -e "${YELLOW}Step 2: Configure with coverage enabled${NC}"
cmake -B ${BUILD_DIR} \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_COVERAGE=ON \
    -DBUILD_TESTS=ON \
    -DBUILD_EXAMPLES=ON

echo -e "${YELLOW}Step 3: Build with coverage${NC}"
cmake --build ${BUILD_DIR} --parallel

echo -e "${YELLOW}Step 4: Initialize coverage baseline${NC}"
lcov --capture --initial --directory ${BUILD_DIR} --output-file ${COVERAGE_DIR}/baseline.info \
    --ignore-errors inconsistent,unsupported \
    --exclude '/opt/homebrew/*' --exclude '/usr/*'

echo -e "${YELLOW}Step 5: Run tests${NC}"
cd ${BUILD_DIR}
echo "Running simple tests..."
./tests/simple_test

echo "Running unit tests..."
./tests/unit_tests

echo "Running examples (for additional coverage)..."
./examples/basic_example > /dev/null
./examples/buffer_operations > /dev/null

cd ..

echo -e "${YELLOW}Step 6: Capture coverage data${NC}"
lcov --capture --directory ${BUILD_DIR} --output-file ${COVERAGE_DIR}/test.info \
    --ignore-errors inconsistent,unsupported \
    --exclude '/opt/homebrew/*' --exclude '/usr/*'

echo -e "${YELLOW}Step 7: Combine baseline and test coverage${NC}"
lcov --add-tracefile ${COVERAGE_DIR}/baseline.info \
     --add-tracefile ${COVERAGE_DIR}/test.info \
     --output-file ${COVERAGE_DIR}/${LCOV_OUTPUT}

echo -e "${YELLOW}Step 8: Remove system and external library coverage${NC}"
lcov --remove ${COVERAGE_DIR}/${LCOV_OUTPUT} \
     '/usr/*' \
     '/opt/*' \
     '/opt/homebrew/*' \
     '*/boost/*' \
     '*/examples/*' \
     '*/tests/*' \
     --output-file ${COVERAGE_DIR}/${LCOV_OUTPUT} \
     --ignore-errors inconsistent,unsupported

echo -e "${YELLOW}Step 9: Generate HTML report${NC}"
genhtml ${COVERAGE_DIR}/${LCOV_OUTPUT} \
    --output-directory ${COVERAGE_DIR}/html \
    --title "BufferLib Code Coverage Report" \
    --show-details \
    --legend \
    --demangle-cpp

echo -e "${GREEN}Coverage report generated successfully!${NC}"
echo ""
echo "Summary:"
lcov --summary ${COVERAGE_DIR}/${LCOV_OUTPUT}

echo ""
echo -e "${GREEN}View the HTML report:${NC}"
echo "  Open: ${COVERAGE_DIR}/html/index.html"
echo ""

# Try to open the report automatically
if command -v open &> /dev/null; then
    # macOS
    echo "Opening coverage report..."
    open ${COVERAGE_DIR}/html/index.html
elif command -v xdg-open &> /dev/null; then
    # Linux
    echo "Opening coverage report..."
    xdg-open ${COVERAGE_DIR}/html/index.html
else
    echo "Open ${COVERAGE_DIR}/html/index.html in your browser to view the report"
fi 