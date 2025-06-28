#include <iostream>
#include <cassert>
#include <vector>
#include "../Buffer.h"

// Simple test framework macros
#define TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        std::cerr << "ASSERTION FAILED: " << message << " at line " << __LINE__ << std::endl; \
        return false; \
    }

#define RUN_TEST(test_func) \
    std::cout << "Running " << #test_func << "..." << std::endl; \
    if (test_func()) { \
        std::cout << "PASSED: " << #test_func << std::endl; \
    } else { \
        std::cout << "FAILED: " << #test_func << std::endl; \
        return 1; \
    }

bool test_buffer_construction() {
    // Test default constructor
    ufs::Buffer buffer1;
    TEST_ASSERT(buffer1.GetSectorCount() == 0x10000, "Default constructor sector count");
    TEST_ASSERT(buffer1.GetBytesPerSector() == 512, "Default constructor bytes per sector");
    
    // Test parameterized constructor
    ufs::Buffer buffer2(100, 1024);
    TEST_ASSERT(buffer2.GetSectorCount() == 100, "Parameterized constructor sector count");
    TEST_ASSERT(buffer2.GetBytesPerSector() == 1024, "Parameterized constructor bytes per sector");
    TEST_ASSERT(buffer2.GetTotalBytes() == 102400, "Total bytes calculation");
    
    // Test copy constructor
    buffer2.FillIncrementing(0x42);
    ufs::Buffer buffer3(buffer2);
    TEST_ASSERT(buffer3.GetSectorCount() == 100, "Copy constructor sector count");
    TEST_ASSERT(buffer3.GetBytesPerSector() == 1024, "Copy constructor bytes per sector");
    
    ufs::CompareResult result = buffer2.CompareTo(buffer3);
    TEST_ASSERT(result.AreEqual(), "Copy constructor data integrity");
    
    return true;
}

bool test_fill_operations() {
    ufs::Buffer buffer(10, 512);
    
    // Test fill zeros
    buffer.FillZeros();
    TEST_ASSERT(buffer.IsAllZeros(), "Fill zeros");
    
    // Test fill ones
    buffer.FillOnes();
    for (size_t i = 0; i < 10; ++i) {
        TEST_ASSERT(buffer.GetByte(i) == 0xFF, "Fill ones verification");
    }
    
    // Test fill with specific value
    buffer.Fill(0xAA);
    for (size_t i = 0; i < 10; ++i) {
        TEST_ASSERT(buffer.GetByte(i) == 0xAA, "Fill with specific value");
    }
    
    // Test incrementing pattern
    buffer.FillIncrementing();
    for (size_t i = 0; i < 10; ++i) {
        TEST_ASSERT(buffer.GetByte(i) == (i % 256), "Incrementing pattern");
    }
    
    // Test decrementing pattern
    buffer.FillDecrementing();
    for (size_t i = 0; i < 10; ++i) {
        TEST_ASSERT(buffer.GetByte(i) == (255 - (i % 256)), "Decrementing pattern");
    }
    
    return true;
}

bool test_data_access() {
    ufs::Buffer buffer(10, 512);
    buffer.FillZeros();
    
    // Test byte operations
    buffer.SetByte(0, 0x12);
    TEST_ASSERT(buffer.GetByte(0) == 0x12, "Byte set/get");
    
    // Test word operations (little endian)
    buffer.SetWord(1, 0x3456);
    TEST_ASSERT(buffer.GetWord(1) == 0x3456, "Word set/get");
    TEST_ASSERT(buffer.GetByte(1) == 0x56, "Word little endian low byte");
    TEST_ASSERT(buffer.GetByte(2) == 0x34, "Word little endian high byte");
    
    // Test dword operations
    buffer.SetDWord(4, 0x789ABCDE);
    TEST_ASSERT(buffer.GetDWord(4) == 0x789ABCDE, "DWord set/get");
    
    // Test qword operations
    buffer.SetQWord(8, 0x123456789ABCDEF0ULL);
    TEST_ASSERT(buffer.GetQWord(8) == 0x123456789ABCDEF0ULL, "QWord set/get");
    
    // Test big endian operations
    buffer.SetWordBigEndian(20, 0x1234);
    TEST_ASSERT(buffer.GetWordBigEndian(20) == 0x1234, "Word big endian set/get");
    TEST_ASSERT(buffer.GetByte(20) == 0x12, "Word big endian high byte");
    TEST_ASSERT(buffer.GetByte(21) == 0x34, "Word big endian low byte");
    
    return true;
}

bool test_bit_operations() {
    ufs::Buffer buffer(10, 512);
    buffer.FillZeros();
    
    // Set a byte with known bit pattern
    buffer.SetByte(0, 0b10101010);
    
    // Test bit access
    TEST_ASSERT(buffer.GetByteBit(0, 0) == 0, "Bit 0 should be 0");
    TEST_ASSERT(buffer.GetByteBit(0, 1) == 1, "Bit 1 should be 1");
    TEST_ASSERT(buffer.GetByteBit(0, 2) == 0, "Bit 2 should be 0");
    TEST_ASSERT(buffer.GetByteBit(0, 3) == 1, "Bit 3 should be 1");
    
    // Test word bit operations
    buffer.SetWord(2, 0b1010101010101010);
    TEST_ASSERT(buffer.GetWordBit(2, 0) == 0, "Word bit 0 should be 0");
    TEST_ASSERT(buffer.GetWordBit(2, 1) == 1, "Word bit 1 should be 1");
    TEST_ASSERT(buffer.GetWordBit(2, 15) == 1, "Word bit 15 should be 1");
    
    return true;
}

bool test_buffer_comparison() {
    ufs::Buffer buffer1(5, 512);
    ufs::Buffer buffer2(5, 512);
    
    // Test equal buffers
    buffer1.FillIncrementing();
    buffer2.FillIncrementing();
    
    ufs::CompareResult result = buffer1.CompareTo(buffer2);
    TEST_ASSERT(result.AreEqual(), "Equal buffers comparison");
    
    // Test unequal buffers
    buffer2.SetByte(100, 0xFF);
    result = buffer1.CompareTo(buffer2);
    TEST_ASSERT(!result.AreEqual(), "Unequal buffers comparison");
    TEST_ASSERT(result.GetFirstDifferenceOffset() == 100, "First difference offset");
    TEST_ASSERT(result.GetExpectedValue() == buffer1.GetByte(100), "Expected value");
    TEST_ASSERT(result.GetActualValue() == 0xFF, "Actual value");
    
    return true;
}

bool test_random_operations() {
    ufs::Buffer buffer1(5, 512);
    ufs::Buffer buffer2(5, 512);
    
    // Test seeded random - same seed should produce same result
    buffer1.FillRandomSeeded(12345);
    buffer2.FillRandomSeeded(12345);
    
    ufs::CompareResult result = buffer1.CompareTo(buffer2);
    TEST_ASSERT(result.AreEqual(), "Seeded random with same seed");
    
    // Test seeded random - different seeds should produce different results
    buffer1.FillRandomSeeded(12345);
    buffer2.FillRandomSeeded(54321);
    
    result = buffer1.CompareTo(buffer2);
    TEST_ASSERT(!result.AreEqual(), "Seeded random with different seeds");
    
    return true;
}

bool test_copy_operations() {
    ufs::Buffer source(10, 512);
    ufs::Buffer dest(10, 512);
    
    // Fill source with pattern
    source.FillIncrementing(0x10);
    dest.FillZeros();
    
    // Test full copy
    source.CopyTo(dest);
    ufs::CompareResult result = source.CompareTo(dest);
    TEST_ASSERT(result.AreEqual(), "Full buffer copy");
    
    // Test partial copy
    ufs::Buffer dest2(10, 512);
    dest2.FillZeros();
    source.CopyTo(dest2, 2, 2, 3);  // Copy 3 sectors starting from sector 2
    
    result = source.CompareTo(dest2, 2, 2, 3);
    TEST_ASSERT(result.AreEqual(), "Partial buffer copy");
    
    return true;
}

bool test_resize_operations() {
    ufs::Buffer buffer(5, 512);
    size_t originalSize = buffer.GetTotalBytes();
    
    // Fill with pattern
    buffer.FillIncrementing();
    
    // Resize larger
    buffer.Resize(10);
    TEST_ASSERT(buffer.GetSectorCount() == 10, "Resize larger sector count");
    TEST_ASSERT(buffer.GetTotalBytes() == 5120, "Resize larger total bytes");
    
    // Verify original data is preserved
    for (size_t i = 0; i < originalSize; ++i) {
        TEST_ASSERT(buffer.GetByte(i) == (i % 256), "Data preserved after resize larger");
    }
    
    // Resize smaller
    buffer.Resize(3);
    TEST_ASSERT(buffer.GetSectorCount() == 3, "Resize smaller sector count");
    TEST_ASSERT(buffer.GetTotalBytes() == 1536, "Resize smaller total bytes");
    
    return true;
}

bool test_utility_functions() {
    ufs::Buffer buffer(5, 512);
    
    // Test checksum
    buffer.FillIncrementing();
    UInt8 checksum1 = buffer.CalculateChecksumByte(0, buffer.GetTotalBytes());
    
    buffer.SetByte(0, buffer.GetByte(0) + 1);
    UInt8 checksum2 = buffer.CalculateChecksumByte(0, buffer.GetTotalBytes());
    
    TEST_ASSERT(checksum1 != checksum2, "Checksum changes with data");
    
    // Test bit count
    buffer.FillZeros();
    UInt64 bitCount = buffer.GetBitCount();
    TEST_ASSERT(bitCount == 0, "Bit count of zeros");
    
    buffer.FillOnes();
    bitCount = buffer.GetBitCount();
    TEST_ASSERT(bitCount == buffer.GetTotalBytes() * 8, "Bit count of ones");
    
    return true;
}

int main() {
    std::cout << "Running BufferLib Unit Tests\n";
    std::cout << "============================\n\n";
    
    try {
        RUN_TEST(test_buffer_construction);
        RUN_TEST(test_fill_operations);
        RUN_TEST(test_data_access);
        RUN_TEST(test_bit_operations);
        RUN_TEST(test_buffer_comparison);
        RUN_TEST(test_random_operations);
        RUN_TEST(test_copy_operations);
        RUN_TEST(test_resize_operations);
        RUN_TEST(test_utility_functions);
        
        std::cout << "\nAll unit tests passed successfully!\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Unit test failed with error: " << e.what() << std::endl;
        return 1;
    }
} 