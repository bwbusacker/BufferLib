#!/bin/bash
# Build script for BufferLib C++ library

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Default values
BUILD_TYPE="Release"
BUILD_EXAMPLES="ON"
BUILD_TESTS="ON"
CLEAN_BUILD=false
INSTALL=false
RUN_TESTS=false
BUILD_DIR="build"
INSTALL_PREFIX=""

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            echo "BufferLib Build Script"
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  -h, --help              Show this help message"
            echo "  -d, --debug             Build in Debug mode (default: Release)"
            echo "  -c, --clean             Clean build directory before building"
            echo "  -i, --install           Install after building"
            echo "  -t, --run-tests         Run tests after building"
            echo "  --no-examples           Don't build examples"
            echo "  --no-tests              Don't build tests"
            echo "  --build-dir DIR         Use custom build directory (default: build)"
            echo "  --prefix DIR            Set installation prefix"
            echo ""
            echo "Examples:"
            echo "  $0                      # Basic build"
            echo "  $0 -d -t               # Debug build and run tests"
            echo "  $0 -c -i               # Clean build and install"
            echo "  $0 --prefix /usr/local # Build and set install prefix"
            exit 0
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        -i|--install)
            INSTALL=true
            shift
            ;;
        -t|--run-tests)
            RUN_TESTS=true
            shift
            ;;
        --no-examples)
            BUILD_EXAMPLES="OFF"
            shift
            ;;
        --no-tests)
            BUILD_TESTS="OFF"
            RUN_TESTS=false
            shift
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --prefix)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        *)
            print_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

print_status "BufferLib Build Configuration:"
echo "  Build Type: $BUILD_TYPE"
echo "  Build Examples: $BUILD_EXAMPLES"
echo "  Build Tests: $BUILD_TESTS"
echo "  Build Directory: $BUILD_DIR"
echo "  Clean Build: $CLEAN_BUILD"
echo "  Install: $INSTALL"
echo "  Run Tests: $RUN_TESTS"
if [[ -n "$INSTALL_PREFIX" ]]; then
    echo "  Install Prefix: $INSTALL_PREFIX"
fi
echo ""

# Check for required tools
print_status "Checking for required tools..."

if ! command -v cmake &> /dev/null; then
    print_error "CMake not found. Please install CMake 3.10 or higher."
    exit 1
fi

CMAKE_VERSION=$(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
print_status "Found CMake version: $CMAKE_VERSION"

if ! command -v make &> /dev/null && ! command -v ninja &> /dev/null; then
    print_error "No build system found. Please install make or ninja."
    exit 1
fi

# Clean build directory if requested
if [[ "$CLEAN_BUILD" == true ]]; then
    print_status "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
if [[ ! -d "$BUILD_DIR" ]]; then
    print_status "Creating build directory: $BUILD_DIR"
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Configure CMake
print_status "Configuring with CMake..."
CMAKE_ARGS=(
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    -DBUILD_EXAMPLES="$BUILD_EXAMPLES"
    -DBUILD_TESTS="$BUILD_TESTS"
)

if [[ -n "$INSTALL_PREFIX" ]]; then
    CMAKE_ARGS+=(-DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX")
fi

# Detect generator
if command -v ninja &> /dev/null; then
    CMAKE_ARGS+=(-G Ninja)
    GENERATOR="Ninja"
else
    GENERATOR="Make"
fi

print_status "Using $GENERATOR build system"

if ! cmake "${CMAKE_ARGS[@]}" ..; then
    print_error "CMake configuration failed!"
    exit 1
fi

print_success "CMake configuration completed successfully"

# Build
print_status "Building BufferLib..."
if ! cmake --build . --config "$BUILD_TYPE"; then
    print_error "Build failed!"
    exit 1
fi

print_success "Build completed successfully"

# Run tests if requested
if [[ "$RUN_TESTS" == true && "$BUILD_TESTS" == "ON" ]]; then
    print_status "Running tests..."
    
    echo ""
    echo "=== Running Simple Tests ==="
    if [[ -f "tests/simple_test" ]]; then
        ./tests/simple_test
    else
        print_warning "Simple test executable not found"
    fi
    
    echo ""
    echo "=== Running Unit Tests ==="
    if [[ -f "tests/unit_tests" ]]; then
        ./tests/unit_tests
    else
        print_warning "Unit test executable not found"
    fi
    
    print_success "All tests completed"
fi

# Install if requested
if [[ "$INSTALL" == true ]]; then
    print_status "Installing BufferLib..."
    if ! cmake --build . --target install; then
        print_error "Installation failed!"
        exit 1
    fi
    print_success "Installation completed successfully"
fi

# Show build artifacts
echo ""
print_status "Build artifacts:"
if [[ -f "libuffer.a" ]]; then
    echo "  Static library: $BUILD_DIR/libuffer.a"
fi

if [[ "$BUILD_EXAMPLES" == "ON" ]]; then
    echo "  Examples:"
    [[ -f "examples/basic_example" ]] && echo "    $BUILD_DIR/examples/basic_example"
    [[ -f "examples/buffer_operations" ]] && echo "    $BUILD_DIR/examples/buffer_operations"
fi

if [[ "$BUILD_TESTS" == "ON" ]]; then
    echo "  Tests:"
    [[ -f "tests/simple_test" ]] && echo "    $BUILD_DIR/tests/simple_test"
    [[ -f "tests/unit_tests" ]] && echo "    $BUILD_DIR/tests/unit_tests"
fi

echo ""
print_success "BufferLib build process completed successfully!"

if [[ "$RUN_TESTS" == false && "$BUILD_TESTS" == "ON" ]]; then
    echo ""
    print_status "To run tests manually:"
    echo "  cd $BUILD_DIR"
    echo "  ./tests/simple_test"
    echo "  ./tests/unit_tests"
fi

if [[ "$BUILD_EXAMPLES" == "ON" ]]; then
    echo ""
    print_status "To run examples:"
    echo "  cd $BUILD_DIR"
    echo "  ./examples/basic_example"
    echo "  ./examples/buffer_operations"
fi 