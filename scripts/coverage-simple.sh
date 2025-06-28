#!/bin/bash

# Simplified Code Coverage Report Generator for BufferLib
# Works better with clang/LLVM on macOS

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

echo -e "${BLUE}BufferLib Code Coverage Report Generator (Simple)${NC}"
echo "=================================================="

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

echo -e "${YELLOW}Step 4: Run tests to generate coverage data${NC}"
cd ${BUILD_DIR}
echo "Running simple tests..."
./tests/simple_test

echo "Running unit tests..."
./tests/unit_tests

echo "Running examples..."
./examples/basic_example > /dev/null
./examples/buffer_operations > /dev/null

cd ..

echo -e "${YELLOW}Step 5: Capture coverage data${NC}"
if command -v lcov &> /dev/null; then
    # Try with lcov if available
    lcov --capture --directory ${BUILD_DIR} --output-file ${COVERAGE_DIR}/${LCOV_OUTPUT} \
        --ignore-errors inconsistent,unsupported,corrupt \
        --exclude '/opt/homebrew/*' \
        --exclude '/usr/*' \
        --exclude '/Library/*' \
        --exclude '*/boost/*' \
        --exclude '*/examples/*' \
        --exclude '*/tests/*' \
        --quiet || echo "lcov failed, trying alternative approach..."
    
    if [ -f ${COVERAGE_DIR}/${LCOV_OUTPUT} ]; then
        echo -e "${YELLOW}Step 6: Generate HTML report${NC}"
        genhtml ${COVERAGE_DIR}/${LCOV_OUTPUT} \
            --output-directory ${COVERAGE_DIR}/html \
            --title "BufferLib Code Coverage Report" \
            --show-details \
            --legend \
            --demangle-cpp \
            --quiet
        
        echo -e "${GREEN}Coverage report generated successfully!${NC}"
        echo ""
        echo "Summary:"
        lcov --summary ${COVERAGE_DIR}/${LCOV_OUTPUT}
        
        echo ""
        echo -e "${GREEN}View the HTML report:${NC}"
        echo "  Open: ${COVERAGE_DIR}/html/index.html"
        
        # Try to open the report
        if command -v open &> /dev/null; then
            echo "Opening coverage report..."
            open ${COVERAGE_DIR}/html/index.html
        fi
    else
        echo -e "${RED}lcov failed to generate coverage report${NC}"
        USE_GCOV=1
    fi
else
    echo -e "${YELLOW}lcov not available, using gcov directly${NC}"
    USE_GCOV=1
fi

# Fallback to gcov if lcov failed
if [ "$USE_GCOV" = "1" ]; then
    echo -e "${YELLOW}Step 6: Using gcov directly${NC}"
    cd ${BUILD_DIR}
    
    echo "Generating gcov reports..."
    find . -name "*.gcda" -exec gcov {} \; > /dev/null 2>&1
    
    echo "Coverage files generated:"
    find . -name "*.gcov" | grep -E "(Buffer|Random32|Utils|CompareResult)" | head -10
    
    echo ""
    echo -e "${GREEN}Coverage analysis completed!${NC}"
    echo "Individual .gcov files are in ${BUILD_DIR}/"
    echo "Look for files like Buffer.cpp.gcov, Random32.cpp.gcov, etc."
    
    cd ..
fi

echo ""
echo -e "${BLUE}Coverage analysis completed!${NC}" 