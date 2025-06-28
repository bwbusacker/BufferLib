#include <iostream>
#include <boost/thread/mutex.hpp>
#include <boost/format.hpp>
#include <boost/random.hpp>

int main() {
    std::cout << "Testing boost headers..." << std::endl;
    
    // Test boost::format
    boost::format fmt("Hello %1%");
    fmt % "World";
    std::cout << "boost::format: " << fmt.str() << std::endl;
    
    // Test boost::random
    boost::random::mt19937 gen;
    boost::random::uniform_int_distribution<> dist(1, 100);
    int random_num = dist(gen);
    std::cout << "boost::random: " << random_num << std::endl;
    
    // Test boost::mutex
    boost::mutex mtx;
    boost::lock_guard<boost::mutex> lock(mtx);
    std::cout << "boost::mutex: OK" << std::endl;
    
    std::cout << "All boost components working!" << std::endl;
    return 0;
} 