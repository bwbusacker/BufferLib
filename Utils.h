#pragma once
#ifndef _UTILS_H_
#define _UTILS_H_

#include <iostream>
#include <sstream>
#include <string>

namespace ufs {
namespace utils {

/// <summary>
/// Initialize a string stream for hexadecimal output
/// </summary>
inline void InitHexStringStream(std::stringstream& ss) {
    ss << std::hex << std::uppercase;
}

/// <summary>
/// Convert bytes to hex string
/// </summary>
std::string BytesToHexString(const void* data, size_t length);

/// <summary>
/// Convert single byte to hex string
/// </summary>
std::string ByteToHexString(unsigned char byte);

/// <summary>
/// Convert value to hex string
/// </summary>
template<typename T>
std::string Hex(T value) {
    std::stringstream ss;
    InitHexStringStream(ss);
    ss << "0x" << value;
    return ss.str();
}

} // namespace utils
} // namespace ufs

#endif // _UTILS_H_ 