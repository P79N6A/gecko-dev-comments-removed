







#include "compiler/compilerdebug.h"

#include <stdarg.h>
#include <stdio.h>

#include "compiler/ParseHelper.h"

static const int kTraceBufferLen = 1024;

#ifdef TRACE_ENABLED
extern "C" {
void Trace(const char *format, ...) {
    if (!format) return;

    TParseContext* parseContext = GetGlobalParseContext();
    if (parseContext) {
        char buf[kTraceBufferLen];
        va_list args;
        va_start(args, format);
        vsnprintf(buf, kTraceBufferLen, format, args);
        va_end(args);

        parseContext->infoSink.debug << buf;
    }
}
}  
#endif  

