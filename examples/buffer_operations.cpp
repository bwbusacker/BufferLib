#include <iostream>
#include "../Buffer.h"

int main() {
    try {
        std::cout << "BufferLib Advanced Operations Example\n";
        std::cout << "====================================\n\n";

        // Create two buffers
        ufs::Buffer buffer1(5, 512);  // 5 sectors, 512 bytes each
        ufs::Buffer buffer2(5, 512);  // Same size

        std::cout << "Created two buffers of " << buffer1.GetTotalBytes() 
                  << " bytes each\n\n";

        // Fill both with the same pattern
        std::cout << "Filling both buffers with incrementing pattern...\n";
        buffer1.FillIncrementing(0x10);  // Start from 0x10
        buffer2.FillIncrementing(0x10);  // Same pattern

        // Compare the buffers
        std::cout << "Comparing buffers...\n";
        ufs::CompareResult result1 = buffer1.CompareTo(buffer2);
        std::cout << "Comparison result: " << result1.ToString() << "\n\n";

        // Modify one buffer
        std::cout << "Modifying buffer2 at position 100...\n";
        buffer2.SetByte(100, 0xFF);

        // Compare again
        ufs::CompareResult result2 = buffer1.CompareTo(buffer2);
        std::cout << "Comparison result after modification: " << result2.ToString() << "\n\n";

        // Copy operations
        std::cout << "Testing copy operations...\n";
        ufs::Buffer buffer3(3, 512);  // Smaller buffer
        buffer3.FillZeros();

        // Copy from buffer1 to buffer3
        buffer1.CopyTo(buffer3, 0, 0, 3);  // Copy 3 sectors
        std::cout << "Copied 3 sectors from buffer1 to buffer3\n";

        // Verify copy
        ufs::CompareResult copyResult = buffer1.CompareTo(buffer3, 0, 0, 3);
        std::cout << "Copy verification: " << copyResult.ToString() << "\n\n";

        // Resize operations
        std::cout << "Testing resize operations...\n";
        std::cout << "Buffer3 size before resize: " << buffer3.GetTotalBytes() << " bytes\n";
        buffer3.Resize(10);  // Resize to 10 sectors
        std::cout << "Buffer3 size after resize: " << buffer3.GetTotalBytes() << " bytes\n\n";

        // Random seeded operations
        std::cout << "Testing seeded random operations...\n";
        buffer1.FillRandomSeeded(12345);  // Use specific seed
        buffer2.FillRandomSeeded(12345);  // Same seed

        ufs::CompareResult randomResult = buffer1.CompareTo(buffer2);
        std::cout << "Seeded random buffers comparison: " << randomResult.ToString() << "\n\n";

        // Bit operations
        std::cout << "Testing bit operations...\n";
        UInt8 byte = buffer1.GetByte(0);
        UInt8 bit = buffer1.GetByteBit(0, 3);  // Get bit 3 of byte 0
        std::cout << "Byte 0: 0x" << std::hex << (int)byte 
                  << ", Bit 3: " << std::dec << (int)bit << "\n";

        UInt16 word = buffer1.GetWord(0);
        UInt8 wordBit = buffer1.GetWordBit(0, 10);  // Get bit 10 of word 0
        std::cout << "Word 0: 0x" << std::hex << word 
                  << ", Bit 10: " << std::dec << (int)wordBit << "\n\n";

        // Display some buffer statistics
        std::cout << "Buffer statistics:\n";
        std::cout << "Buffer1 bit count: " << buffer1.GetBitCount() << "\n";
        std::cout << "Buffer1 checksum: 0x" << std::hex 
                  << (int)buffer1.CalculateChecksumByte(0, buffer1.GetTotalBytes()) 
                  << std::dec << "\n\n";

        std::cout << "Advanced operations example completed successfully!\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 