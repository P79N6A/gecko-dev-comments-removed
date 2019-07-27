





#include "common/angleutils.h"

#include <vector>

std::string FormatString(const char *fmt, va_list vararg)
{
    static std::vector<char> buffer(512);

    
    int len = vsnprintf(&buffer[0], buffer.size(), fmt, vararg);
    if (len < 0 || static_cast<size_t>(len) >= buffer.size())
    {
        
        len = vsnprintf(NULL, 0, fmt, vararg);
        buffer.resize(len + 1);

        
        vsnprintf(&buffer[0], buffer.size(), fmt, vararg);
    }

    return std::string(buffer.data(), len);
}

std::string FormatString(const char *fmt, ...)
{
    va_list vararg;
    va_start(vararg, fmt);
    std::string result = FormatString(fmt, vararg);
    va_end(vararg);
    return result;
}
