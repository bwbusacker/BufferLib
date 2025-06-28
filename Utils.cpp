#include "Utils.h"
#include <iomanip>

namespace ufs {
namespace utils {

std::string BytesToHexString(const void* data, size_t length) {
    std::stringstream ss;
    InitHexStringStream(ss);
    
    const unsigned char* bytes = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < length; ++i) {
        ss << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
    }
    
    return ss.str();
}

std::string ByteToHexString(unsigned char byte) {
    std::stringstream ss;
    InitHexStringStream(ss);
    ss << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    return ss.str();
}

} // namespace utils
} // namespace ufs 