#include <iostream>
#include "../Buffer.h"

int main() {
    try {
        std::cout << "BufferLib Basic Example\n";
        std::cout << "=====================\n\n";

        // Create a buffer with 10 sectors of 512 bytes each
        ufs::Buffer buffer(10, 512);
        
        std::cout << "Created buffer with " << buffer.GetSectorCount() 
                  << " sectors of " << buffer.GetBytesPerSector() 
                  << " bytes each\n";
        std::cout << "Total buffer size: " << buffer.GetTotalBytes() << " bytes\n\n";

        // Fill the buffer with incrementing values
        std::cout << "Filling buffer with incrementing pattern...\n";
        buffer.FillIncrementing();

        // Read some bytes
        std::cout << "First 10 bytes: ";
        for (size_t i = 0; i < 10; ++i) {
            std::cout << "0x" << std::hex << (int)buffer.GetByte(i) << " ";
        }
        std::cout << std::dec << "\n\n";

        // Fill with random data
        std::cout << "Filling buffer with random data...\n";
        buffer.FillRandom();

        // Read some bytes again
        std::cout << "First 10 bytes after random fill: ";
        for (size_t i = 0; i < 10; ++i) {
            std::cout << "0x" << std::hex << (int)buffer.GetByte(i) << " ";
        }
        std::cout << std::dec << "\n\n";

        // Fill with zeros
        std::cout << "Filling buffer with zeros...\n";
        buffer.FillZeros();

        // Check if all zeros
        std::cout << "Buffer is all zeros: " << (buffer.IsAllZeros() ? "Yes" : "No") << "\n\n";

        // Set some specific bytes
        buffer.SetByte(0, 0xAA);
        buffer.SetByte(1, 0xBB);
        buffer.SetWord(2, 0xCCDD);
        buffer.SetDWord(4, 0xEEFF0011);

        std::cout << "Set specific values:\n";
        std::cout << "Byte 0: 0x" << std::hex << (int)buffer.GetByte(0) << "\n";
        std::cout << "Byte 1: 0x" << (int)buffer.GetByte(1) << "\n";
        std::cout << "Word 2: 0x" << buffer.GetWord(2) << "\n";
        std::cout << "DWord 4: 0x" << buffer.GetDWord(4) << std::dec << "\n\n";

        std::cout << "Example completed successfully!\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 