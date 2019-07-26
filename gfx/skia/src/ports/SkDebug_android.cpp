








#include "SkTypes.h"

static const size_t kBufferSize = 256;

#define LOG_TAG "skia"
#include <android/log.h>

static bool gSkDebugToStdOut = false;

extern "C" void AndroidSkDebugToStdOut(bool debugToStdOut) {
    gSkDebugToStdOut = debugToStdOut;
}

void SkDebugf(const char format[], ...) {
    va_list args;
    va_start(args, format);
    __android_log_vprint(ANDROID_LOG_DEBUG, LOG_TAG, format, args);

    
    
    if (gSkDebugToStdOut) {
        vprintf(format, args);
    }

    va_end(args);
}
