#!/bin/bash

# Performance Benchmark Runner for BufferLib
# Runs benchmarks with different configurations and generates reports

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
RESULTS_DIR="benchmark-results"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

echo -e "${BLUE}BufferLib Performance Benchmark Runner${NC}"
echo "======================================"

# Create results directory
mkdir -p ${RESULTS_DIR}

echo -e "${YELLOW}System Information:${NC}"
echo "Date: $(date)"
echo "Platform: $(uname -a)"
if command -v lscpu &> /dev/null; then
    echo "CPU: $(lscpu | grep 'Model name' | cut -d: -f2 | xargs)"
    echo "CPU cores: $(lscpu | grep '^CPU(s):' | cut -d: -f2 | xargs)"
elif command -v sysctl &> /dev/null; then
    echo "CPU: $(sysctl -n machdep.cpu.brand_string)"
    echo "CPU cores: $(sysctl -n hw.ncpu)"
fi
echo ""

# Function to run benchmarks with specific configuration
run_benchmark() {
    local build_type=$1
    local label=$2
    local build_type_lower=$(echo "$build_type" | tr '[:upper:]' '[:lower:]')
    local build_dir="build-perf-${build_type_lower}"
    
    echo -e "${YELLOW}=== Running ${label} Benchmarks ===${NC}"
    
    # Clean and create build directory
    rm -rf ${build_dir}
    
    # Configure
    echo "Configuring ${label} build..."
    cmake -B ${build_dir} \
        -DCMAKE_BUILD_TYPE=${build_type} \
        -DBUILD_TESTS=ON \
        -DBUILD_EXAMPLES=ON \
        -DENABLE_COVERAGE=OFF \
        > /dev/null
    
    # Build
    echo "Building ${label}..."
    cmake --build ${build_dir} --parallel > /dev/null
    
    # Run benchmarks
    echo "Running ${label} benchmarks..."
    local result_file="${RESULTS_DIR}/benchmark_${build_type_lower}_${TIMESTAMP}.txt"
    
    echo "=== BufferLib Performance Benchmarks ===" > ${result_file}
    echo "Build Type: ${build_type}" >> ${result_file}
    echo "Date: $(date)" >> ${result_file}
    echo "Platform: $(uname -a)" >> ${result_file}
    echo "" >> ${result_file}
    
    cd ${build_dir}
    ./tests/performance_tests >> ../${result_file} 2>&1
    cd ..
    
    echo -e "${GREEN}${label} benchmarks completed: ${result_file}${NC}"
    echo ""
}

# Run different build configurations
run_benchmark "Release" "Release (Optimized)"
run_benchmark "Debug" "Debug (Unoptimized)"

# Optional: Run with different compiler optimizations
if [ "$1" = "--extended" ]; then
    echo -e "${YELLOW}=== Running Extended Benchmarks ===${NC}"
    
    # Release with specific optimizations
    echo "Configuring Release with -O3..."
    cmake -B build-perf-o3 \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS="-O3 -march=native" \
        -DBUILD_TESTS=ON \
        -DBUILD_EXAMPLES=ON \
        -DENABLE_COVERAGE=OFF \
        > /dev/null
    
    cmake --build build-perf-o3 --parallel > /dev/null
    
    local result_file="${RESULTS_DIR}/benchmark_o3_${TIMESTAMP}.txt"
    echo "=== BufferLib Performance Benchmarks (O3) ===" > ${result_file}
    echo "Build Type: Release with -O3 -march=native" >> ${result_file}
    echo "Date: $(date)" >> ${result_file}
    echo "" >> ${result_file}
    
    cd build-perf-o3
    ./tests/performance_tests >> ../${result_file} 2>&1
    cd ..
    
    echo -e "${GREEN}Extended benchmarks completed: ${result_file}${NC}"
fi

# Generate summary report
echo -e "${YELLOW}=== Generating Summary Report ===${NC}"
summary_file="${RESULTS_DIR}/summary_${TIMESTAMP}.md"

echo "# BufferLib Performance Benchmark Summary" > ${summary_file}
echo "" >> ${summary_file}
echo "**Date:** $(date)" >> ${summary_file}
echo "**Platform:** $(uname -a)" >> ${summary_file}
echo "" >> ${summary_file}

for result_file in ${RESULTS_DIR}/benchmark_*_${TIMESTAMP}.txt; do
    if [ -f "$result_file" ]; then
        build_type=$(basename "$result_file" | cut -d_ -f2)
        build_type_cap=$(echo "$build_type" | sed 's/./\U&/')
        echo "## ${build_type_cap} Build Results" >> ${summary_file}
        echo "" >> ${summary_file}
        echo '```' >> ${summary_file}
        # Extract key performance metrics
        grep -A 20 "Fill Operation Performance:" "$result_file" | head -25 >> ${summary_file} || true
        echo '```' >> ${summary_file}
        echo "" >> ${summary_file}
    fi
done

echo -e "${GREEN}Summary report generated: ${summary_file}${NC}"

# Display quick comparison
echo -e "${YELLOW}=== Quick Performance Comparison ===${NC}"
echo "Key Operations (Release vs Debug):"
echo "-----------------------------------"

for operation in "Fill with Zeros" "Fill Incrementing" "Sequential Byte Read" "Buffer Copy"; do
    echo "Operation: $operation"
    for build in release debug; do
        file="${RESULTS_DIR}/benchmark_${build}_${TIMESTAMP}.txt"
        if [ -f "$file" ]; then
            timing=$(grep "$operation" "$file" | head -1 | sed -n 's/.*μ=\([0-9.]*\)μs.*/\1/p' || echo "N/A")
            build_cap=$(echo "$build" | sed 's/./\U&/')
            printf "  %-10s: %10s μs\n" "$build_cap" "$timing"
        fi
    done
    echo ""
done

echo -e "${GREEN}Performance benchmark analysis complete!${NC}"
echo ""
echo "Results saved in: ${RESULTS_DIR}/"
echo "View summary: cat ${summary_file}"

# Optional: Open results if on macOS
if command -v open &> /dev/null && [ -f "${summary_file}" ]; then
    echo ""
    echo "Opening summary report..."
    open "${summary_file}"
fi 