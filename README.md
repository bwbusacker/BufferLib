# BufferLib - C++ Buffer Management Library

BufferLib is a comprehensive C++ library for managing memory buffers with sector-based operations, designed primarily for storage device testing and data manipulation applications.

## Features

- **Sector-based Buffer Management**: Work with buffers organized into sectors of configurable size
- **Multiple Data Fill Patterns**: Incrementing, decrementing, random, fixed value patterns
- **Data Access Methods**: Byte, word, dword, and qword access with bit-level operations
- **Buffer Comparison**: Compare buffers and get detailed difference reports
- **Copy Operations**: Efficient buffer-to-buffer copying with partial sector support
- **Random Number Generation**: Seeded and unseeded random data generation using boost::random::taus88
- **Endianness Support**: Both little-endian and big-endian data access
- **Resizing**: Dynamic buffer resizing with data preservation
- **File I/O**: Save and load buffers to/from files (binary and compressed formats)

## Requirements

- C++17 compatible compiler (GCC 7+, Clang 6+, MSVC 2017+)
- CMake 3.10 or higher
- Boost libraries (thread, system components)
- OpenMP (optional, for parallel operations)

## Building

### Quick Build

```bash
# Create build directory
mkdir build && cd build

# Configure
cmake ..

# Build
cmake --build .

# Run tests
./tests/simple_test
./tests/unit_tests

# Run examples
./examples/basic_example
./examples/buffer_operations
```

### Build Options

- `BUILD_EXAMPLES=ON/OFF` - Build example programs (default: ON)
- `BUILD_TESTS=ON/OFF` - Build test programs (default: ON)

```bash
cmake -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF ..
```

### Installation

```bash
# Install library and headers
cmake --build . --target install

# Or specify custom installation directory
cmake -DCMAKE_INSTALL_PREFIX=/path/to/install ..
cmake --build . --target install
```

## Usage

### Basic Example

```cpp
#include <iostream>
#include "Buffer.h"

int main() {
    // Create a buffer with 10 sectors of 512 bytes each
    ufs::Buffer buffer(10, 512);
    
    // Fill with incrementing pattern
    buffer.FillIncrementing();
    
    // Access data
    UInt8 byte = buffer.GetByte(0);
    UInt16 word = buffer.GetWord(0);
    
    // Set data
    buffer.SetByte(0, 0xAA);
    buffer.SetDWord(4, 0x12345678);
    
    // Compare with another buffer
    ufs::Buffer buffer2(10, 512);
    buffer2.FillIncrementing();
    
    ufs::CompareResult result = buffer.CompareTo(buffer2);
    if (!result.AreEqual()) {
        std::cout << "Buffers differ at offset: " 
                  << result.GetFirstDifferenceOffset() << std::endl;
    }
    
    return 0;
}
```

### Using the Library in Your Project

#### With CMake

```cmake
find_package(BufferLib REQUIRED)
target_link_libraries(your_target PRIVATE BufferLib::BufferLib)
```

#### Manual Linking

```bash
g++ -std=c++17 your_program.cpp -lbuffer -lboost_thread -lboost_system
```

## API Overview

### Buffer Creation

```cpp
ufs::Buffer buffer;                    // Default: 65536 sectors × 512 bytes
ufs::Buffer buffer(1000);              // 1000 sectors × 512 bytes
ufs::Buffer buffer(1000, 1024);        // 1000 sectors × 1024 bytes
ufs::Buffer buffer(other_buffer);      // Copy constructor
```

### Fill Operations

```cpp
buffer.FillZeros();                    // Fill with zeros
buffer.FillOnes();                     // Fill with 0xFF
buffer.Fill(0xAA);                     // Fill with specific value
buffer.FillIncrementing();             // Fill with 0,1,2,3...
buffer.FillDecrementing();             // Fill with 255,254,253...
buffer.FillRandom();                   // Fill with random data
buffer.FillRandomSeeded(12345);        // Fill with seeded random data
```

### Data Access

```cpp
// Byte operations
UInt8 byte = buffer.GetByte(index);
buffer.SetByte(index, value);
UInt8 bit = buffer.GetByteBit(index, bit_number);

// Word operations (16-bit)
UInt16 word = buffer.GetWord(index);
buffer.SetWord(index, value);
UInt16 wordBE = buffer.GetWordBigEndian(index);

// DWord operations (32-bit)
UInt32 dword = buffer.GetDWord(index);
buffer.SetDWord(index, value);

// QWord operations (64-bit)
UInt64 qword = buffer.GetQWord(index);
buffer.SetQWord(index, value);
```

### Buffer Operations

```cpp
// Comparison
ufs::CompareResult result = buffer1.CompareTo(buffer2);
bool equal = result.AreEqual();
size_t firstDiff = result.GetFirstDifferenceOffset();

// Copying
buffer1.CopyTo(buffer2);                          // Full copy
buffer1.CopyTo(buffer2, startSector, destSector, count);  // Partial copy

// Resizing
buffer.Resize(newSectorCount);
buffer.Resize(newSectorCount, newBytesPerSector);

// Information
size_t sectors = buffer.GetSectorCount();
size_t bytesPerSector = buffer.GetBytesPerSector();
size_t totalBytes = buffer.GetTotalBytes();
bool allZeros = buffer.IsAllZeros();
```

## Project Structure

```
BufferProj/
├── Buffer.h              # Main buffer class header
├── Buffer.cpp            # Main buffer class implementation
├── TypeDefs.h            # Type definitions
├── Errors.h              # Custom exception classes
├── Printable.h           # Base printable interface
├── Random32.h/.cpp       # Random number generator
├── Utils.h/.cpp          # Utility functions
├── CompareResult.h/.cpp  # Buffer comparison results
├── CMakeLists.txt        # Main build configuration
├── Config.cmake.in       # CMake package configuration
├── examples/             # Example programs
│   ├── basic_example.cpp
│   ├── buffer_operations.cpp
│   └── CMakeLists.txt
├── tests/                # Test programs
│   ├── simple_test.cpp
│   ├── unit_tests.cpp
│   └── CMakeLists.txt
└── README.md             # This file
```

## Running Tests

The library includes comprehensive tests to verify functionality:

```bash
# Simple functionality tests
./tests/simple_test

# Comprehensive unit tests
./tests/unit_tests
```

## Examples

Several example programs demonstrate library usage:

```bash
# Basic buffer operations
./examples/basic_example

# Advanced operations and comparisons
./examples/buffer_operations
```

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Performance Notes

- The library uses OpenMP for parallel operations when available
- Buffer operations are optimized for large data sets
- Memory is allocated with alignment considerations for optimal performance
- Boost libraries are used for thread-safe operations

## Troubleshooting

### Common Build Issues

1. **Boost not found**: Ensure Boost development packages are installed
   ```bash
   # Ubuntu/Debian
   sudo apt-get install libboost-dev libboost-thread-dev libboost-system-dev
   
   # macOS (Homebrew)
   brew install boost
   
   # Windows (vcpkg)
   vcpkg install boost-thread boost-system
   ```

2. **CMake version too old**: Update CMake to 3.10 or higher

3. **Compiler issues**: Ensure C++17 support is available

### Memory Usage

- Default buffer size is 32MB (65536 sectors × 512 bytes)
- Consider system memory limitations when creating large buffers
- Use resize operations carefully with large buffers

For more detailed information, see the inline documentation in the header files. 