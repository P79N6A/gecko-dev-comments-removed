








#include "SkTypes.h"

#ifdef SK_BUILD_FOR_BREW

static const size_t kBufferSize = 256;

#include <AEEStdLib.h>
#include <stdarg.h>

void SkDebugf(const char format[], ...) {
    char    buffer[kBufferSize + 1];
    va_list args;
    va_start(args, format);
    VSNPRINTF(buffer, kBufferSize, format, args);
    va_end(args);
    DBGPRINTF(buffer);
}

#endif SK_BUILD_FOR_BREW
