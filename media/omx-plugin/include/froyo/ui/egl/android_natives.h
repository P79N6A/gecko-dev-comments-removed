















#ifndef ANDROID_ANDROID_NATIVES_H
#define ANDROID_ANDROID_NATIVES_H

#include <sys/types.h>
#include <string.h>

#include <hardware/gralloc.h>

#ifdef __cplusplus
extern "C" {
#endif



#define ANDROID_NATIVE_MAKE_CONSTANT(a,b,c,d) \
    (((unsigned)(a)<<24)|((unsigned)(b)<<16)|((unsigned)(c)<<8)|(unsigned)(d))

#define ANDROID_NATIVE_WINDOW_MAGIC \
    ANDROID_NATIVE_MAKE_CONSTANT('_','w','n','d')

#define ANDROID_NATIVE_BUFFER_MAGIC \
    ANDROID_NATIVE_MAKE_CONSTANT('_','b','f','r')



struct android_native_buffer_t;



typedef struct android_native_base_t
{
    
    int magic;
    
    
    int version;

    void* reserved[4];

    
    void (*incRef)(struct android_native_base_t* base);
    void (*decRef)(struct android_native_base_t* base);
} android_native_base_t;




enum {
    NATIVE_WINDOW_WIDTH     = 0,
    NATIVE_WINDOW_HEIGHT    = 1,
    NATIVE_WINDOW_FORMAT    = 2,
};


enum {
    NATIVE_WINDOW_SET_USAGE  = 0,
    NATIVE_WINDOW_CONNECT    = 1,
    NATIVE_WINDOW_DISCONNECT = 2
};


enum {
    NATIVE_WINDOW_API_EGL = 1
};

typedef struct android_native_window_t 
{
#ifdef __cplusplus
    android_native_window_t()
        : flags(0), minSwapInterval(0), maxSwapInterval(0), xdpi(0), ydpi(0)
    {
        common.magic = ANDROID_NATIVE_WINDOW_MAGIC;
        common.version = sizeof(android_native_window_t);
        memset(common.reserved, 0, sizeof(common.reserved));
    }
#endif
    
    struct android_native_base_t common;

    
    const uint32_t flags;
    
    
    const int   minSwapInterval;

    
    const int   maxSwapInterval;

    
    const float xdpi;
    const float ydpi;

    
    intptr_t    oem[4];
        

    




    int     (*setSwapInterval)(struct android_native_window_t* window,
                int interval);
    
    






    int     (*dequeueBuffer)(struct android_native_window_t* window, 
                struct android_native_buffer_t** buffer);

    






    int     (*lockBuffer)(struct android_native_window_t* window,
                struct android_native_buffer_t* buffer);
   







    int     (*queueBuffer)(struct android_native_window_t* window,
                struct android_native_buffer_t* buffer);

    




    int     (*query)(struct android_native_window_t* window,
                int what, int* value);
    
    
















    
    int     (*perform)(struct android_native_window_t* window,
                int operation, ... );
    
    void* reserved_proc[3];
} android_native_window_t;












static inline int native_window_set_usage(
        android_native_window_t* window, int usage)
{
    return window->perform(window, NATIVE_WINDOW_SET_USAGE, usage);
}







static inline int native_window_connect(
        android_native_window_t* window, int api)
{
    return window->perform(window, NATIVE_WINDOW_CONNECT, api);
}







static inline int native_window_disconnect(
        android_native_window_t* window, int api)
{
    return window->perform(window, NATIVE_WINDOW_DISCONNECT, api);
}





typedef struct egl_native_pixmap_t
{
    int32_t     version;    
    int32_t     width;
    int32_t     height;
    int32_t     stride;
    uint8_t*    data;
    uint8_t     format;
    uint8_t     rfu[3];
    union {
        uint32_t    compressedFormat;
        int32_t     vstride;
    };
    int32_t     reserved;
} egl_native_pixmap_t;



#ifdef __cplusplus
}
#endif




#ifdef __cplusplus

#include <utils/RefBase.h>

namespace android {





template <typename NATIVE_TYPE, typename TYPE, typename REF>
class EGLNativeBase : public NATIVE_TYPE, public REF
{
protected:
    typedef EGLNativeBase<NATIVE_TYPE, TYPE, REF> BASE;
    EGLNativeBase() : NATIVE_TYPE(), REF() {
        NATIVE_TYPE::common.incRef = incRef;
        NATIVE_TYPE::common.decRef = decRef;
    }
    static inline TYPE* getSelf(NATIVE_TYPE* self) {
        return static_cast<TYPE*>(self);
    }
    static inline TYPE const* getSelf(NATIVE_TYPE const* self) {
        return static_cast<TYPE const *>(self);
    }
    static inline TYPE* getSelf(android_native_base_t* base) {
        return getSelf(reinterpret_cast<NATIVE_TYPE*>(base));
    }
    static inline TYPE const * getSelf(android_native_base_t const* base) {
        return getSelf(reinterpret_cast<NATIVE_TYPE const*>(base));
    }
    static void incRef(android_native_base_t* base) {
        EGLNativeBase* self = getSelf(base);
        self->incStrong(self);
    }
    static void decRef(android_native_base_t* base) {
        EGLNativeBase* self = getSelf(base);
        self->decStrong(self);
    }
};

} 
#endif 



#endif
