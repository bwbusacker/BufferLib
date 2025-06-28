#include "CompareResult.h"
#include <sstream>
#include <iomanip>

namespace ufs {

CompareResult::CompareResult() 
    : _areEqual(true), _firstDifferenceOffset(0), _expectedValue(0), _actualValue(0), _differenceCount(0) {
}

CompareResult::CompareResult(size_t firstDifferenceOffset, UInt8 expectedValue, UInt8 actualValue)
    : _areEqual(false), _firstDifferenceOffset(firstDifferenceOffset), 
      _expectedValue(expectedValue), _actualValue(actualValue), _differenceCount(1) {
}

void CompareResult::AddDifference(size_t offset, UInt8 expectedValue, UInt8 actualValue) {
    if (_areEqual) {
        _areEqual = false;
        _firstDifferenceOffset = offset;
        _expectedValue = expectedValue;
        _actualValue = actualValue;
        _differenceCount = 1;
    } else {
        _differenceCount++;
    }
}

std::string CompareResult::ToString() const {
    std::stringstream ss;
    
    if (_areEqual) {
        ss << "Buffers are equal";
    } else {
        ss << "Buffers are not equal. First difference at offset " << _firstDifferenceOffset
           << ": expected 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') 
           << static_cast<int>(_expectedValue)
           << ", actual 0x" << std::setw(2) << std::setfill('0') 
           << static_cast<int>(_actualValue)
           << ". Total differences: " << std::dec << _differenceCount;
    }
    
    return ss.str();
}

} // namespace ufs 