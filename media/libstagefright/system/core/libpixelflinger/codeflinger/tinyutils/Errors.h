















#ifndef ANDROID_PIXELFLINGER_ERRORS_H
#define ANDROID_PIXELFLINGER_ERRORS_H

#include <sys/types.h>
#include <errno.h>

namespace stagefright {
namespace tinyutils {


typedef int32_t     status_t;






enum {
    NO_ERROR          = 0,    
    NO_MEMORY           = -ENOMEM,
    BAD_VALUE           = -EINVAL,
    BAD_INDEX           = -EOVERFLOW,
    NAME_NOT_FOUND      = -ENOENT,
};


} 
} 
    

    
#endif 
