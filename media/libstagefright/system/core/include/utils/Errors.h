















#ifndef ANDROID_ERRORS_H
#define ANDROID_ERRORS_H

#include <sys/types.h>
#include <errno.h>

namespace stagefright {


#ifdef HAVE_MS_C_RUNTIME
typedef int         status_t;
#else
typedef int32_t     status_t;
#endif










#ifdef _WIN32
# undef NO_ERROR
#endif
 
enum {
    OK                = 0,    
    NO_ERROR          = 0,    
    
    UNKNOWN_ERROR       = 0x80000000,

    NO_MEMORY           = -ENOMEM,
    INVALID_OPERATION   = -ENOSYS,
    BAD_VALUE           = -EINVAL,
    BAD_TYPE            = 0x80000001,
    NAME_NOT_FOUND      = -ENOENT,
    PERMISSION_DENIED   = -EPERM,
    NO_INIT             = -ENODEV,
    ALREADY_EXISTS      = -EEXIST,
    DEAD_OBJECT         = -EPIPE,
    FAILED_TRANSACTION  = 0x80000002,
    JPARKS_BROKE_IT     = -EPIPE,
#if !defined(HAVE_MS_C_RUNTIME)
    BAD_INDEX           = -EOVERFLOW,
    NOT_ENOUGH_DATA     = -ENODATA,
    WOULD_BLOCK         = -EWOULDBLOCK, 
    TIMED_OUT           = -ETIMEDOUT,
    UNKNOWN_TRANSACTION = -EBADMSG,
#else    
    BAD_INDEX           = -E2BIG,
    NOT_ENOUGH_DATA     = 0x80000003,
    WOULD_BLOCK         = 0x80000004,
    TIMED_OUT           = 0x80000005,
    UNKNOWN_TRANSACTION = 0x80000006,
#endif    
    FDS_NOT_ALLOWED     = 0x80000007,
};



#ifdef _WIN32
# define NO_ERROR 0L
#endif

}; 
    

    
#endif 
