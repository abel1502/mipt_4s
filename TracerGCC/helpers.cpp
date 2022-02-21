#include "helpers.h"


std::string helpers::vsprintfxx(const char *fmt, va_list args) {
    va_list args2{};

    va_copy(args2, args);
    int reqSize = vsnprintf(nullptr, 0, fmt, args2);
    va_end(args2);

    assert(reqSize >= 0);
    
    std::string result{};
    result.resize(reqSize + 1);

    int writtenSize = vsnprintf(result.data(), reqSize + 1, fmt, args);

    assert(writtenSize > 0 && writtenSize <= reqSize);

    result.resize(writtenSize);

    return result;
}
