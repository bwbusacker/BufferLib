#pragma once
#ifndef _ERRORS_H_
#define _ERRORS_H_

#include <stdexcept>
#include <string>

namespace ufs {

class OutOfRangeError : public std::out_of_range {
public:
    explicit OutOfRangeError(const std::string& message) : std::out_of_range(message) {}
    explicit OutOfRangeError(const char* message) : std::out_of_range(message) {}
};

class InvalidArgumentError : public std::invalid_argument {
public:
    explicit InvalidArgumentError(const std::string& message) : std::invalid_argument(message) {}
    explicit InvalidArgumentError(const char* message) : std::invalid_argument(message) {}
};

// Alias for compatibility
using ArgumentError = InvalidArgumentError;

class RuntimeError : public std::runtime_error {
public:
    explicit RuntimeError(const std::string& message) : std::runtime_error(message) {}
    explicit RuntimeError(const char* message) : std::runtime_error(message) {}
};

} // namespace ufs

#endif // _ERRORS_H_ 