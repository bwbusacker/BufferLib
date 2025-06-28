# BufferLib - High-Performance C++ Buffer Management Library

BufferLib is a sophisticated, high-performance C++ library designed for efficient buffer management and data manipulation operations. Originally developed for storage device testing at Micron Technology, it provides comprehensive buffer operations with sector-based memory management, multiple data access patterns, and extensive testing capabilities.

## Features

### Core Buffer Operations
- **Sector-based Memory Management**: Default 512-byte sectors with configurable sizes
- **Multiple Data Access Patterns**: Support for byte, word, dword, and qword operations
- **Endianness Support**: Both little-endian and big-endian data access
- **Advanced Fill Operations**: Incrementing, decrementing, random, and custom patterns
- **High-Performance Random Number Generation**: Using boost::random::taus88 for optimal performance

### Data Manipulation
- **Buffer Comparison**: Detailed difference reporting with customizable limits
- **Copy Operations**: Efficient copying with partial sector support
- **Dynamic Resizing**: Memory-safe resizing with data preservation
- **Bit-Level Operations**: Comprehensive bit manipulation and statistics
- **File I/O**: Direct buffer-to-file operations

### Performance & Quality
- **Optimized Performance**: Designed for high-throughput data operations
- **Comprehensive Testing**: 100% code coverage with unit and integration tests
- **Performance Benchmarking**: Built-in performance testing suite
- **Memory Safety**: Exception-safe operations with proper error handling
- **Cross-Platform**: Supports Linux, macOS, and Windows

## Quick Start

### Prerequisites
- C++11 compatible compiler (GCC 4.9+, Clang 3.4+, MSVC 2015+)
- CMake 3.12 or later
- Boost libraries (thread, system, random, format)

### Building the Library

```bash
# Clone the repository
git clone https://github.com/bwbusacker/BufferLib.git
cd BufferLib

# Build with default settings (Release mode)
./build.sh

# Build with options
./build.sh -d -t  # Debug build with tests
./build.sh -c -i  # Clean build and install
```

### Basic Usage

```cpp
#include "Buffer.h"

// Create a buffer with 1000 sectors (512KB)
ufs::Buffer buffer(1000);

// Fill with incrementing pattern
buffer.FillIncrementing();

// Access data
UInt32 value = buffer.GetDWord(0);  // Read 32-bit value
buffer.SetByte(100, 0xFF);          // Write single byte

// Copy operations
ufs::Buffer copy(buffer);           // Copy constructor
buffer.CopyTo(copy);               // Copy to existing buffer

// Comparison
auto result = buffer.CompareTo(copy);
if (result.AreEqual()) {
    std::cout << "Buffers are identical" << std::endl;
}
```

## Performance Testing

BufferLib includes comprehensive performance benchmarking capabilities:

### Running Performance Tests

```bash
# Quick performance test
cd build && ./tests/performance_tests

# Comprehensive benchmark analysis
./scripts/run-benchmarks.sh

# Extended benchmarks with compiler optimizations
./scripts/run-benchmarks.sh --extended
```

### Performance Benchmarks Include:
- **Buffer Creation/Destruction**: Memory allocation performance
- **Fill Operations**: Data pattern generation speeds (zeros, incrementing, random)
- **Data Access**: Sequential read/write performance for different data types
- **Copy Operations**: Memory-to-memory transfer rates
- **Buffer Comparison**: Comparison algorithm performance
- **Random Number Generation**: RNG performance metrics
- **Memory Operations**: Resize and copy constructor performance

### Sample Performance Results (Apple M4 Pro):
```
Fill Operation Performance:
---------------------------
Fill with Zeros                         : μ=2.00    μs σ=0.00   μs (244 GB/s)
Fill Incrementing Pattern               : μ=6.60    μs σ=0.49   μs (81 GB/s)
Fill Random Data                        : μ=298.90  μs σ=7.03   μs (1.6 GB/s)

Data Access Performance:
------------------------
Sequential Byte Read (100k operations)  : μ=242.20  μs σ=7.69   μs
Sequential DWord Read (100k operations) : μ=46.00   μs σ=0.00   μs
Buffer Copy (512 KB)                    : μ=7.00    μs σ=0.00   μs (70 GB/s)
```

## Testing

BufferLib maintains 100% code coverage with comprehensive testing:

```bash
# Run all tests
./build.sh -t

# Run specific test suites
cd build
./tests/simple_test          # Basic functionality
./tests/unit_tests           # Comprehensive unit tests
./tests/performance_tests    # Performance benchmarks
```

### Test Coverage
- **356/356 lines covered (100%)**
- Unit tests for all public APIs
- Edge case and error condition testing
- Performance regression testing
- Memory safety validation

## API Reference

### Core Classes

#### `ufs::Buffer`
Main buffer class providing sector-based memory management.

**Key Methods:**
- `Buffer(size_t sectors)` - Constructor
- `Fill(UInt8 value)` - Fill with constant value
- `FillIncrementing(UInt8 start = 0)` - Fill with incrementing pattern
- `FillRandom()` - Fill with random data
- `GetByte(size_t offset)` - Read byte value
- `SetDWord(size_t offset, UInt32 value)` - Write 32-bit value
- `CompareTo(const Buffer& other)` - Compare buffers
- `CopyTo(Buffer& dest)` - Copy to another buffer
- `Resize(size_t newSectors)` - Resize buffer

#### `ufs::Random32`
High-performance random number generator using boost::random::taus88.

#### `ufs::CompareResult`
Detailed buffer comparison results with difference analysis.

## Build System

### CMake Options
- `BUILD_TESTS=ON/OFF` - Build test suite (default: ON)
- `BUILD_EXAMPLES=ON/OFF` - Build example programs (default: ON)
- `ENABLE_COVERAGE=ON/OFF` - Enable code coverage (default: OFF)
- `CMAKE_BUILD_TYPE` - Build type (Debug/Release)

### Installation
```bash
# Install to system location
./build.sh -i

# Custom installation prefix
./build.sh --prefix /usr/local
```

## CI/CD Integration

BufferLib includes comprehensive GitHub Actions workflows:
- **Multi-platform testing** (Ubuntu, macOS, Windows)
- **Code coverage reporting** with Codecov integration
- **Performance benchmarking** with regression detection
- **Automated releases** with artifact generation
- **Code quality analysis** with cppcheck and clang-tidy

## Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature-name`
3. Make changes with tests: ensure 100% code coverage is maintained
4. Run performance benchmarks: `./scripts/run-benchmarks.sh`
5. Submit a pull request

### Development Workflow
- All changes must include comprehensive tests
- Performance regressions are automatically detected
- Code coverage must remain at 100%
- Follow existing code style and documentation standards

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Performance Optimization Notes

BufferLib is optimized for high-performance applications:
- Uses boost::random::taus88 for faster random number generation
- Employs efficient memory patterns for cache optimization
- Leverages compiler optimizations and SIMD when available
- Includes OpenMP support for parallel operations (when available)
- Designed with minimal overhead for embedded systems

## Changelog

### v1.0.0 (2025-06-28)
- Initial release with full API
- 100% code coverage achieved
- Comprehensive performance benchmarking suite
- Multi-platform CI/CD pipeline
- Complete documentation and examples

## Acknowledgments

Originally developed for storage device testing at Micron Technology. Evolved into a general-purpose high-performance buffer management library with modern C++ practices and comprehensive testing infrastructure. 