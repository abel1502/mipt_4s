#include "helpers.h"


namespace helpers {


std::string vsprintfxx(const char *fmt, va_list args) {
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


std::string htmlEncode(const std::string_view &data) {
    std::string result{};

    result.reserve(data.size());
    for (unsigned i = 0; i != data.size(); ++i) {
        switch (data[i]) {
            case '&' : result.append("&amp;");     break;
            case '\"': result.append("&quot;");    break;
            case '\'': result.append("&apos;");    break;
            case '<' : result.append("&lt;");      break;
            case '>' : result.append("&gt;");      break;
            default  : result.append(&data[i], 1); break;
        }
    }
    
    return result;
}


}
