








#include "libGLESv2/Error.h"

#include "common/angleutils.h"

#include <cstdarg>

namespace gl
{

Error::Error(GLenum errorCode)
    : mCode(errorCode),
      mMessage()
{
}

Error::Error(GLenum errorCode, const char *msg, ...)
    : mCode(errorCode),
      mMessage()
{
    va_list vararg;
    va_start(vararg, msg);
    mMessage = FormatString(msg, vararg);
    va_end(vararg);
}

Error::Error(const Error &other)
    : mCode(other.mCode),
      mMessage(other.mMessage)
{
}

Error &Error::operator=(const Error &other)
{
    mCode = other.mCode;
    mMessage = other.mMessage;
    return *this;
}

}
