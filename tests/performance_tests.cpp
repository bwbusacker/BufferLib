#include "../Buffer.h"
#include "../Random32.h"
#include "../Utils.h"
#include <chrono>
#include <iostream>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <numeric>

// Performance testing framework
class PerformanceBenchmark {
private:
    std::string name;
    std::vector<double> timings;
    
public:
    PerformanceBenchmark(const std::string& testName) : name(testName) {}
    
    template<typename Func>
    void run(Func&& func, int iterations = 10) {
        timings.clear();
        timings.reserve(iterations);
        
        // Warm up
        func();
        
        for (int i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            func();
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            timings.push_back(duration.count());
        }
    }
    
    void printResults() {
        if (timings.empty()) return;
        
        double sum = std::accumulate(timings.begin(), timings.end(), 0.0);
        double mean = sum / timings.size();
        
        std::vector<double> sorted_timings = timings;
        std::sort(sorted_timings.begin(), sorted_timings.end());
        
        double median = sorted_timings[sorted_timings.size() / 2];
        double min = sorted_timings.front();
        double max = sorted_timings.back();
        
        // Calculate standard deviation
        double variance = 0.0;
        for (double timing : timings) {
            variance += (timing - mean) * (timing - mean);
        }
        variance /= timings.size();
        double stddev = std::sqrt(variance);
        
        std::cout << std::left << std::setw(40) << name << ": ";
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "μ=" << std::setw(8) << mean << "μs ";
        std::cout << "σ=" << std::setw(7) << stddev << "μs ";
        std::cout << "min=" << std::setw(8) << min << "μs ";
        std::cout << "max=" << std::setw(8) << max << "μs ";
        std::cout << "median=" << std::setw(8) << median << "μs" << std::endl;
    }
};

// Helper function to format throughput
void printThroughput(const std::string& operation, size_t dataSize, double timeUs) {
    double mbPerSec = (dataSize / (1024.0 * 1024.0)) / (timeUs / 1000000.0);
    std::cout << std::left << std::setw(40) << operation << ": ";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << mbPerSec << " MB/s" << std::endl;
}

int main() {
    std::cout << "BufferLib Performance Benchmarks" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << std::endl;
    
    // Test configuration
    const size_t SMALL_SECTORS = 100;    // 51.2 KB
    const size_t MEDIUM_SECTORS = 1000;  // 512 KB  
    const size_t LARGE_SECTORS = 10000;  // 5.12 MB
    const size_t BYTES_PER_SECTOR = 512;
    const int ITERATIONS = 10;
    
    std::cout << "Test Configuration:" << std::endl;
    std::cout << "- Small buffer: " << SMALL_SECTORS << " sectors (" << (SMALL_SECTORS * BYTES_PER_SECTOR / 1024) << " KB)" << std::endl;
    std::cout << "- Medium buffer: " << MEDIUM_SECTORS << " sectors (" << (MEDIUM_SECTORS * BYTES_PER_SECTOR / 1024) << " KB)" << std::endl;
    std::cout << "- Large buffer: " << LARGE_SECTORS << " sectors (" << (LARGE_SECTORS * BYTES_PER_SECTOR / 1024 / 1024) << " MB)" << std::endl;
    std::cout << "- Iterations per test: " << ITERATIONS << std::endl;
    std::cout << std::endl;
    
    // === Buffer Creation/Destruction Performance ===
    std::cout << "Buffer Creation/Destruction Performance:" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    {
        PerformanceBenchmark bench("Small Buffer Creation (100 sectors)");
        bench.run([&]() {
            ufs::Buffer buffer(SMALL_SECTORS);
        }, ITERATIONS);
        bench.printResults();
    }
    
    {
        PerformanceBenchmark bench("Medium Buffer Creation (1000 sectors)");
        bench.run([&]() {
            ufs::Buffer buffer(MEDIUM_SECTORS);
        }, ITERATIONS);
        bench.printResults();
    }
    
    {
        PerformanceBenchmark bench("Large Buffer Creation (10000 sectors)");
        bench.run([&]() {
            ufs::Buffer buffer(LARGE_SECTORS);
        }, ITERATIONS);
        bench.printResults();
    }
    
    // === Fill Operation Performance ===
    std::cout << std::endl << "Fill Operation Performance:" << std::endl;
    std::cout << "---------------------------" << std::endl;
    
    ufs::Buffer testBuffer(MEDIUM_SECTORS);
    size_t dataSize = MEDIUM_SECTORS * BYTES_PER_SECTOR;
    
    {
        PerformanceBenchmark bench("Fill with Zeros");
        bench.run([&]() {
            testBuffer.Fill(0);
        }, ITERATIONS);
        bench.printResults();
        
        auto start = std::chrono::high_resolution_clock::now();
        testBuffer.Fill(0);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        printThroughput("Fill Zeros Throughput", dataSize, duration.count());
    }
    
    {
        PerformanceBenchmark bench("Fill Incrementing Pattern");
        bench.run([&]() {
            testBuffer.FillIncrementing();
        }, ITERATIONS);
        bench.printResults();
        
        auto start = std::chrono::high_resolution_clock::now();
        testBuffer.FillIncrementing();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        printThroughput("Fill Incrementing Throughput", dataSize, duration.count());
    }
    
    {
        PerformanceBenchmark bench("Fill Random Data");
        bench.run([&]() {
            testBuffer.FillRandom();
        }, ITERATIONS);
        bench.printResults();
        
        auto start = std::chrono::high_resolution_clock::now();
        testBuffer.FillRandom();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        printThroughput("Fill Random Throughput", dataSize, duration.count());
    }
    
    {
        PerformanceBenchmark bench("Fill Seeded Random Data");
        bench.run([&]() {
            testBuffer.FillRandomSeeded(12345);
        }, ITERATIONS);
        bench.printResults();
    }
    
    // === Data Access Performance ===
    std::cout << std::endl << "Data Access Performance:" << std::endl;
    std::cout << "------------------------" << std::endl;
    
    testBuffer.FillIncrementing();
    const size_t ACCESS_COUNT = 100000;
    
    {
        PerformanceBenchmark bench("Sequential Byte Read (100k operations)");
        bench.run([&]() {
            volatile UInt8 sum = 0;
            for (size_t i = 0; i < ACCESS_COUNT && i < testBuffer.GetTotalBytes(); ++i) {
                sum += testBuffer.GetByte(i);
            }
        }, ITERATIONS);
        bench.printResults();
    }
    
    {
        PerformanceBenchmark bench("Sequential Word Read (100k operations)");
        bench.run([&]() {
            volatile UInt16 sum = 0;
            for (size_t i = 0; i < ACCESS_COUNT && i < testBuffer.GetTotalBytes() - 1; i += 2) {
                sum += testBuffer.GetWord(i);
            }
        }, ITERATIONS);
        bench.printResults();
    }
    
    {
        PerformanceBenchmark bench("Sequential DWord Read (100k operations)");
        bench.run([&]() {
            volatile UInt32 sum = 0;
            for (size_t i = 0; i < ACCESS_COUNT && i < testBuffer.GetTotalBytes() - 3; i += 4) {
                sum += testBuffer.GetDWord(i);
            }
        }, ITERATIONS);
        bench.printResults();
    }
    
    {
        PerformanceBenchmark bench("Sequential Byte Write (100k operations)");
        bench.run([&]() {
            for (size_t i = 0; i < ACCESS_COUNT && i < testBuffer.GetTotalBytes(); ++i) {
                testBuffer.SetByte(i, static_cast<UInt8>(i & 0xFF));
            }
        }, ITERATIONS);
        bench.printResults();
    }
    
    // === Copy Operation Performance ===
    std::cout << std::endl << "Copy Operation Performance:" << std::endl;
    std::cout << "---------------------------" << std::endl;
    
    ufs::Buffer sourceBuffer(MEDIUM_SECTORS);
    ufs::Buffer destBuffer(MEDIUM_SECTORS);
    sourceBuffer.FillIncrementing();
    
    {
        PerformanceBenchmark bench("Buffer Copy (512 KB)");
        bench.run([&]() {
            sourceBuffer.CopyTo(destBuffer);
        }, ITERATIONS);
        bench.printResults();
        
        auto start = std::chrono::high_resolution_clock::now();
        sourceBuffer.CopyTo(destBuffer);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        printThroughput("Copy Throughput", dataSize, duration.count());
    }
    
    // === Buffer Comparison Performance ===
    std::cout << std::endl << "Buffer Comparison Performance:" << std::endl;
    std::cout << "-------------------------------" << std::endl;
    
    ufs::Buffer buffer1(MEDIUM_SECTORS);
    ufs::Buffer buffer2(MEDIUM_SECTORS);
    buffer1.FillIncrementing();
    buffer2.FillIncrementing();
    
    {
        PerformanceBenchmark bench("Buffer Comparison (identical)");
        bench.run([&]() {
            auto result = buffer1.CompareTo(buffer2);
        }, ITERATIONS);
        bench.printResults();
    }
    
    // Make buffers different
    buffer2.SetByte(MEDIUM_SECTORS * BYTES_PER_SECTOR / 2, 0xFF);
    
    {
        PerformanceBenchmark bench("Buffer Comparison (different)");
        bench.run([&]() {
            auto result = buffer1.CompareTo(buffer2);
        }, ITERATIONS);
        bench.printResults();
    }
    
    // === Random Number Generation Performance ===
    std::cout << std::endl << "Random Number Generation Performance:" << std::endl;
    std::cout << "-------------------------------------" << std::endl;
    
    {
        PerformanceBenchmark bench("Random32 Generation (1M numbers)");
        bench.run([&]() {
            ufs::Random32 rng(12345);
            volatile UInt32 sum = 0;
            for (int i = 0; i < 1000000; ++i) {
                sum += rng.Next();
            }
        }, ITERATIONS);
        bench.printResults();
    }
    
    {
        PerformanceBenchmark bench("Random32 Byte Generation (1M bytes)");
        bench.run([&]() {
            ufs::Random32 rng(12345);
            volatile UInt32 sum = 0;
            for (int i = 0; i < 1000000; ++i) {
                sum += rng.NextByte();
            }
        }, ITERATIONS);
        bench.printResults();
    }
    
    // === Memory Allocation Performance ===
    std::cout << std::endl << "Memory Operations Performance:" << std::endl;
    std::cout << "-------------------------------" << std::endl;
    
    {
        PerformanceBenchmark bench("Buffer Resize (1000->2000 sectors)");
        bench.run([&]() {
            ufs::Buffer buffer(1000);
            buffer.FillIncrementing();
            buffer.Resize(2000);
        }, ITERATIONS);
        bench.printResults();
    }
    
    {
        PerformanceBenchmark bench("Buffer Copy Constructor");
        bench.run([&]() {
            ufs::Buffer original(1000);
            original.FillIncrementing();
            ufs::Buffer copy(original);
        }, ITERATIONS);
        bench.printResults();
    }
    
    // === Summary ===
    std::cout << std::endl << "Performance Benchmarks Complete!" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << "All timings are in microseconds (μs)" << std::endl;
    std::cout << "Throughput measurements are in MB/s" << std::endl;
    std::cout << "Statistics: μ=mean, σ=std deviation, min/max/median" << std::endl;
    
    return 0;
} 