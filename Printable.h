#pragma once
#ifndef _PRINTABLE_H_
#define _PRINTABLE_H_

#include <string>

namespace ufs {

/// <summary>
/// Base class for objects that can be converted to string representation
/// </summary>
class Printable {
public:
    virtual ~Printable() = default;
    
    /// <summary>
    /// Convert the object to a string representation
    /// </summary>
    virtual std::string ToString() const = 0;
};

} // namespace ufs

#endif // _PRINTABLE_H_ 