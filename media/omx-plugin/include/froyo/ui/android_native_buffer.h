















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
#endif

    struct android_native_base_t common;

    int width;
    int height;
    int stride;
    int format;
    int usage;
    
    void* reserved[2];

    buffer_handle_t handle;

    void* reserved_proc[8];
} android_native_buffer_t;




#ifdef __cplusplus
}
#endif



#endif
