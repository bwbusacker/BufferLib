#include "Random32.h"
#include <chrono>
#include <boost/random.hpp>

namespace ufs {

Random32::Random32() : _generator(std::chrono::system_clock::now().time_since_epoch().count()), _isSeeded(false) {
}

Random32::Random32(UInt32 seed) : _generator(seed), _isSeeded(true) {
}

Random32::Random32(const Random32& other) : _generator(other._generator), _isSeeded(other._isSeeded) {
}

void Random32::Seed(UInt32 seed) {
    _generator.seed(seed);
    _isSeeded = true;
}

UInt32 Random32::Next() {
    return _generator();
}

UInt32 Random32::Next(UInt32 max) {
    if (max == 0) return 0;
    boost::random::uniform_int_distribution<UInt32> dist(0, max - 1);
    return dist(_generator);
}

UInt32 Random32::Next(UInt32 min, UInt32 max) {
    if (min >= max) return min;
    boost::random::uniform_int_distribution<UInt32> dist(min, max - 1);
    return dist(_generator);
}

UInt8 Random32::NextByte() {
    return static_cast<UInt8>(Next(256));
}

void Random32::NextBytes(UInt8* buffer, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        buffer[i] = NextByte();
    }
}

} // namespace ufs 