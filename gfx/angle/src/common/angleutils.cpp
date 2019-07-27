





#include "common/angleutils.h"
#include "debug.h"
#include <stdio.h>
#include <vector>

size_t FormatStringIntoVector(const char *fmt, va_list vararg, std::vector<char>& outBuffer)
{
    
    int len = vsnprintf(&(outBuffer.front()), outBuffer.size(), fmt, vararg);
    if (len < 0 || static_cast<size_t>(len) >= outBuffer.size())
    {
        
        len = vsnprintf(NULL, 0, fmt, vararg);
        outBuffer.resize(len + 1);

        
        len = vsnprintf(&(outBuffer.front()), outBuffer.size(), fmt, vararg);
    }
    ASSERT(len >= 0);
    return static_cast<size_t>(len);
}

std::string FormatString(const char *fmt, va_list vararg)
{
    static std::vector<char> buffer(512);

    size_t len = FormatStringIntoVector(fmt, vararg, buffer);
    return std::string(&buffer[0], len);
}

std::string FormatString(const char *fmt, ...)
{
    va_list vararg;
    va_start(vararg, fmt);
    std::string result = FormatString(fmt, vararg);
    va_end(vararg);
    return result;
}
