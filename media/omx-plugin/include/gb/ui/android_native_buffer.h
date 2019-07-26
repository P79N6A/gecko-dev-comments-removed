















#ifndef ANDROID_ANDROID_NATIVES_PRIV_H
#define ANDROID_ANDROID_NATIVES_PRIV_H

#include <ui/egl/android_natives.h>

#ifdef __cplusplus
extern "C" {
#endif



typedef struct android_native_buffer_t
{
#ifdef __cplusplus
    android_native_buffer_t() { 
        common.magic = ANDROID_NATIVE_BUFFER_MAGIC;
        common.version = sizeof(android_native_buffer_t);
        memset(common.reserved, 0, sizeof(common.reserved));
    }

    
    
    void incStrong(const void* id) const {
        common.incRef(const_cast<android_native_base_t*>(&common));
    }
    void decStrong(const void* id) const {
        common.decRef(const_cast<android_native_base_t*>(&common));
    }
#endif

    struct android_native_base_t common;

    int width;
    int height;
    int stride;
    int format;
    int usage;

    
    uint8_t transform;

    uint8_t reserved_bytes[3];
    void* reserved[1];

    buffer_handle_t handle;

    void* reserved_proc[8];
} android_native_buffer_t;




#ifdef __cplusplus
}
#endif



#endif
