#pragma once
#ifndef _COMPARERESULT_H_
#define _COMPARERESULT_H_

#include "TypeDefs.h"
#include "Printable.h"
#include <string>
#include <vector>

namespace ufs {

/// <summary>
/// Represents the result of comparing two buffers
/// </summary>
class CompareResult : public Printable {
public:
    /// <summary>
    /// Default constructor - indicates buffers are equal
    /// </summary>
    CompareResult();
    
    /// <summary>
    /// Constructor for unequal buffers
    /// </summary>
    CompareResult(size_t firstDifferenceOffset, UInt8 expectedValue, UInt8 actualValue);
    
    /// <summary>
    /// Destructor
    /// </summary>
    virtual ~CompareResult() = default;
    
    /// <summary>
    /// Check if the buffers are equal
    /// </summary>
    bool AreEqual() const { return _areEqual; }
    
    /// <summary>
    /// Get the offset of the first difference
    /// </summary>
    size_t GetFirstDifferenceOffset() const { return _firstDifferenceOffset; }
    
    /// <summary>
    /// Get the expected value at the first difference
    /// </summary>
    UInt8 GetExpectedValue() const { return _expectedValue; }
    
    /// <summary>
    /// Get the actual value at the first difference
    /// </summary>
    UInt8 GetActualValue() const { return _actualValue; }
    
    /// <summary>
    /// Get the total number of differences found
    /// </summary>
    size_t GetDifferenceCount() const { return _differenceCount; }
    
    /// <summary>
    /// Add a difference to the result
    /// </summary>
    void AddDifference(size_t offset, UInt8 expectedValue, UInt8 actualValue);
    
    /// <summary>
    /// Convert to string representation
    /// </summary>
    virtual std::string ToString() const override;

private:
    bool _areEqual;
    size_t _firstDifferenceOffset;
    UInt8 _expectedValue;
    UInt8 _actualValue;
    size_t _differenceCount;
};

} // namespace ufs

#endif // _COMPARERESULT_H_ 