















#ifndef ANDROID_TRACE_H
#define ANDROID_TRACE_H

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cutils/compiler.h>
#include <utils/threads.h>
#include "cutils_trace.h"






#define ATRACE_NAME(name) android::ScopedTrace ___tracer(ATRACE_TAG, name)

#define ATRACE_CALL() ATRACE_NAME(__FUNCTION__)

namespace android {

class ScopedTrace {
public:
inline ScopedTrace(uint64_t tag, const char* name)
    : mTag(tag) {
#ifdef HAVE_ANDROID_OS
    atrace_begin(mTag,name);
#endif
}

inline ~ScopedTrace() {
#ifdef HAVE_ANDROID_OS
    atrace_end(mTag);
#endif
}

private:
    uint64_t mTag;
};

}; 

#endif 
