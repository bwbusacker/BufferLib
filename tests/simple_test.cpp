#include <iostream>
#include <cassert>
#include "../Buffer.h"

void test_basic_functionality() {
    std::cout << "Testing basic functionality...\n";
    
    // Test buffer creation
    ufs::Buffer buffer(10, 512);
    assert(buffer.GetSectorCount() == 10);
    assert(buffer.GetBytesPerSector() == 512);
    assert(buffer.GetTotalBytes() == 5120);
    
    // Test fill operations
    buffer.FillZeros();
    assert(buffer.IsAllZeros());
    
    buffer.FillOnes();
    assert(!buffer.IsAllZeros());
    
    // Test byte operations
    buffer.SetByte(0, 0xAA);
    assert(buffer.GetByte(0) == 0xAA);
    
    // Test word operations
    buffer.SetWord(0, 0xBBCC);
    assert(buffer.GetWord(0) == 0xBBCC);
    
    // Test dword operations
    buffer.SetDWord(0, 0xDDEEFF00);
    assert(buffer.GetDWord(0) == 0xDDEEFF00);
    
    std::cout << "Basic functionality tests passed!\n";
}

void test_buffer_comparison() {
    std::cout << "Testing buffer comparison...\n";
    
    ufs::Buffer buffer1(5, 512);
    ufs::Buffer buffer2(5, 512);
    
    // Fill both with same pattern
    buffer1.FillIncrementing();
    buffer2.FillIncrementing();
    
    // Should be equal
    ufs::CompareResult result = buffer1.CompareTo(buffer2);
    assert(result.AreEqual());
    
    // Modify one buffer
    buffer2.SetByte(100, 0xFF);
    
    // Should not be equal
    result = buffer1.CompareTo(buffer2);
    assert(!result.AreEqual());
    assert(result.GetFirstDifferenceOffset() == 100);
    
    std::cout << "Buffer comparison tests passed!\n";
}

void test_random_operations() {
    std::cout << "Testing random operations...\n";
    
    ufs::Buffer buffer1(5, 512);
    ufs::Buffer buffer2(5, 512);
    
    // Fill with same seed
    buffer1.FillRandomSeeded(12345);
    buffer2.FillRandomSeeded(12345);
    
    // Should be equal
    ufs::CompareResult result = buffer1.CompareTo(buffer2);
    assert(result.AreEqual());
    
    // Fill with different seeds
    buffer1.FillRandomSeeded(12345);
    buffer2.FillRandomSeeded(54321);
    
    // Should not be equal (very high probability)
    result = buffer1.CompareTo(buffer2);
    assert(!result.AreEqual());
    
    std::cout << "Random operations tests passed!\n";
}

void test_copy_operations() {
    std::cout << "Testing copy operations...\n";
    
    ufs::Buffer source(5, 512);
    ufs::Buffer dest(5, 512);
    
    // Fill source with pattern
    source.FillIncrementing(0x10);
    dest.FillZeros();
    
    // Copy from source to dest
    source.CopyTo(dest);
    
    // Should be equal
    ufs::CompareResult result = source.CompareTo(dest);
    assert(result.AreEqual());
    
    std::cout << "Copy operations tests passed!\n";
}

void test_resize_operations() {
    std::cout << "Testing resize operations...\n";
    
    ufs::Buffer buffer(5, 512);
    assert(buffer.GetTotalBytes() == 2560);
    
    // Resize to larger
    buffer.Resize(10);
    assert(buffer.GetSectorCount() == 10);
    assert(buffer.GetTotalBytes() == 5120);
    
    // Resize to smaller
    buffer.Resize(3);
    assert(buffer.GetSectorCount() == 3);
    assert(buffer.GetTotalBytes() == 1536);
    
    std::cout << "Resize operations tests passed!\n";
}

int main() {
    try {
        std::cout << "Running BufferLib Simple Tests\n";
        std::cout << "==============================\n\n";
        
        test_basic_functionality();
        test_buffer_comparison();
        test_random_operations();
        test_copy_operations();
        test_resize_operations();
        
        std::cout << "\nAll tests passed successfully!\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with error: " << e.what() << std::endl;
        return 1;
    }
} 