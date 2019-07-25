









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_ALIGNED_MALLOC_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_ALIGNED_MALLOC_H_

#include <stddef.h>

namespace webrtc
{
    void* AlignedMalloc(
        size_t size,
        size_t alignment);
    void AlignedFree(
        void* memBlock);
}

#endif 
