#pragma once
#ifndef _RANDOM32_H_
#define _RANDOM32_H_

#include "TypeDefs.h"
#include <random>
#include <boost/random.hpp>

namespace ufs {

/// <summary>
/// 32-bit random number generator using boost::random::taus88
/// 
/// Uses the Tausworthe generator with improved speed and statistical properties
/// compared to traditional generators like Mersenne Twister.
/// </summary>
class Random32 {
public:
    /// <summary>
    /// Default constructor with random seed
    /// </summary>
    Random32();
    
    /// <summary>
    /// Constructor with specific seed
    /// </summary>
    explicit Random32(UInt32 seed);
    
    /// <summary>
    /// Copy constructor
    /// </summary>
    Random32(const Random32& other);
    
    /// <summary>
    /// Destructor
    /// </summary>
    ~Random32() = default;
    
    /// <summary>
    /// Set the seed for the random number generator
    /// </summary>
    void Seed(UInt32 seed);
    
    /// <summary>
    /// Generate next random UInt32 value
    /// </summary>
    UInt32 Next();
    
    /// <summary>
    /// Generate next random UInt32 value in range [0, max)
    /// </summary>
    UInt32 Next(UInt32 max);
    
    /// <summary>
    /// Generate next random UInt32 value in range [min, max)
    /// </summary>
    UInt32 Next(UInt32 min, UInt32 max);
    
    /// <summary>
    /// Generate next random UInt8 value
    /// </summary>
    UInt8 NextByte();
    
    /// <summary>
    /// Fill buffer with random bytes
    /// </summary>
    void NextBytes(UInt8* buffer, size_t length);
    
    /// <summary>
    /// Check if the generator has been seeded
    /// </summary>
    bool GetIsSeeded() const { return _isSeeded; }
    
    /// <summary>
    /// Get the underlying generator (for compatibility)
    /// </summary>
    const boost::random::taus88& GetGen() const { return _generator; }

private:
    boost::random::taus88 _generator;
    boost::random::uniform_int_distribution<UInt32> _distribution;
    bool _isSeeded;
};

} // namespace ufs

#endif // _RANDOM32_H_ 