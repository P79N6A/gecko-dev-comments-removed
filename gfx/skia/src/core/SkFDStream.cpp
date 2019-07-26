






#include "SkStream.h"

#ifdef SK_BUILD_FOR_WIN


SkFDStream::SkFDStream(int, bool) : fFD(-1), fCloseWhenDone(false) {}
SkFDStream::~SkFDStream() {}
bool SkFDStream::rewind() { return false; }
size_t SkFDStream::read(void*, size_t) { return 0; }

#else

#include <unistd.h>



SkFDStream::SkFDStream(int fileDesc, bool closeWhenDone)
    : fFD(fileDesc), fCloseWhenDone(closeWhenDone) {
}

SkFDStream::~SkFDStream() {
    if (fFD >= 0 && fCloseWhenDone) {
        ::close(fFD);
    }
}

bool SkFDStream::rewind() {
    if (fFD >= 0) {
        off_t value = ::lseek(fFD, 0, SEEK_SET);
#ifdef TRACE_FDSTREAM
        if (value) {
            SkDebugf("xxxxxxxxxxxxxx rewind failed %d\n", value);
        }
#endif
        return value == 0;
    }
    return false;
}

size_t SkFDStream::read(void* buffer, size_t size) {
    if (fFD >= 0) {
        if (buffer == NULL && size == 0) {  
            off_t curr = ::lseek(fFD, 0, SEEK_CUR);
            if (curr < 0) {
#ifdef TRACE_FDSTREAM
                SkDebugf("xxxxxxxxxxxxx lseek failed 0 CURR\n");
#endif
                return 0;   
            }
            off_t size = ::lseek(fFD, 0, SEEK_END);
            if (size < 0) {
#ifdef TRACE_FDSTREAM
                SkDebugf("xxxxxxxxxxxxx lseek failed 0 END\n");
#endif
                size = 0;   
            }
            if (::lseek(fFD, curr, SEEK_SET) != curr) {
                
#ifdef TRACE_FDSTREAM
                SkDebugf("xxxxxxxxxxxxx lseek failed %d SET\n", curr);
#endif
                return 0;
            }
            return (size_t) size;
        } else if (NULL == buffer) {        
            off_t oldCurr = ::lseek(fFD, 0, SEEK_CUR);
            if (oldCurr < 0) {
#ifdef TRACE_FDSTREAM
                SkDebugf("xxxxxxxxxxxxx lseek1 failed %d CUR\n", oldCurr);
#endif
                return 0;   
            }
            off_t newCurr = ::lseek(fFD, size, SEEK_CUR);
            if (newCurr < 0) {
#ifdef TRACE_FDSTREAM
                SkDebugf("xxxxxxxxxxxxx lseek2 failed %d CUR\n", newCurr);
#endif
                return 0;   
            }
            
            return (size_t) (newCurr - oldCurr);
        } else {                            
            ssize_t actual = ::read(fFD, buffer, size);
            
            if (actual < 0) {
#ifdef TRACE_FDSTREAM
                SkDebugf("xxxxxxxxxxxxx read failed %d actual %d\n", size, actual);
#endif
                actual = 0;
            }
            return actual;
        }
    }
    return 0;
}

#endif
