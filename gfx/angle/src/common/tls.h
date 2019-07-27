







#ifndef COMMON_TLS_H_
#define COMMON_TLS_H_

#include "common/platform.h"

#ifdef ANGLE_PLATFORM_WINDOWS


#   ifdef ANGLE_ENABLE_WINDOWS_STORE
#       define TLS_OUT_OF_INDEXES -1
#       ifndef CREATE_SUSPENDED
#           define CREATE_SUSPENDED 0x00000004
#       endif
#   endif
    typedef DWORD TLSIndex;
#   define TLS_INVALID_INDEX (TLS_OUT_OF_INDEXES)
#elif defined(ANGLE_PLATFORM_POSIX)
#   include <pthread.h>
#   include <semaphore.h>
#   include <errno.h>
    typedef pthread_key_t TLSIndex;
#   define TLS_INVALID_INDEX (static_cast<TLSIndex>(-1))
#else
#   error Unsupported platform.
#endif




TLSIndex CreateTLSIndex();
bool DestroyTLSIndex(TLSIndex index);

bool SetTLSValue(TLSIndex index, void *value);
void *GetTLSValue(TLSIndex index);

#endif 
