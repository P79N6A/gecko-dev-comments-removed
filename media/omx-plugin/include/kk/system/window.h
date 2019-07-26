















#ifndef SYSTEM_CORE_INCLUDE_ANDROID_WINDOW_H
#define SYSTEM_CORE_INCLUDE_ANDROID_WINDOW_H

#include <cutils/native_handle.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <sync/sync.h>
#include <sys/cdefs.h>
#include <system/graphics.h>
#include <unistd.h>

__BEGIN_DECLS



#define ANDROID_NATIVE_MAKE_CONSTANT(a,b,c,d) \
    (((unsigned)(a)<<24)|((unsigned)(b)<<16)|((unsigned)(c)<<8)|(unsigned)(d))

#define ANDROID_NATIVE_WINDOW_MAGIC \
    ANDROID_NATIVE_MAKE_CONSTANT('_','w','n','d')

#define ANDROID_NATIVE_BUFFER_MAGIC \
    ANDROID_NATIVE_MAKE_CONSTANT('_','b','f','r')







#define ANDROID_NATIVE_WINDOW_HAS_SYNC 1



typedef const native_handle_t* buffer_handle_t;



typedef struct android_native_rect_t
{
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} android_native_rect_t;



typedef struct android_native_base_t
{
    
    int magic;

    
    int version;

    void* reserved[4];

    
    void (*incRef)(struct android_native_base_t* base);
    void (*decRef)(struct android_native_base_t* base);
} android_native_base_t;

typedef struct ANativeWindowBuffer
{
#ifdef __cplusplus
    ANativeWindowBuffer() {
        common.magic = ANDROID_NATIVE_BUFFER_MAGIC;
        common.version = sizeof(ANativeWindowBuffer);
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

    void* reserved[2];

    buffer_handle_t handle;

    void* reserved_proc[8];
} ANativeWindowBuffer_t;


typedef ANativeWindowBuffer_t android_native_buffer_t;




enum {
    NATIVE_WINDOW_WIDTH     = 0,
    NATIVE_WINDOW_HEIGHT    = 1,
    NATIVE_WINDOW_FORMAT    = 2,

    

















    NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS = 3,

    












    NATIVE_WINDOW_QUEUES_TO_WINDOW_COMPOSER = 4,

    





    NATIVE_WINDOW_CONCRETE_TYPE = 5,


    





    NATIVE_WINDOW_DEFAULT_WIDTH = 6,
    NATIVE_WINDOW_DEFAULT_HEIGHT = 7,

    














































    NATIVE_WINDOW_TRANSFORM_HINT = 8,

    



    NATIVE_WINDOW_CONSUMER_RUNNING_BEHIND = 9
};











enum {
    NATIVE_WINDOW_SET_USAGE                 =  0,
    NATIVE_WINDOW_CONNECT                   =  1,   
    NATIVE_WINDOW_DISCONNECT                =  2,   
    NATIVE_WINDOW_SET_CROP                  =  3,   
    NATIVE_WINDOW_SET_BUFFER_COUNT          =  4,
    NATIVE_WINDOW_SET_BUFFERS_GEOMETRY      =  5,   
    NATIVE_WINDOW_SET_BUFFERS_TRANSFORM     =  6,
    NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP     =  7,
    NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS    =  8,
    NATIVE_WINDOW_SET_BUFFERS_FORMAT        =  9,
    NATIVE_WINDOW_SET_SCALING_MODE          = 10,   
    NATIVE_WINDOW_LOCK                      = 11,   
    NATIVE_WINDOW_UNLOCK_AND_POST           = 12,   
    NATIVE_WINDOW_API_CONNECT               = 13,   
    NATIVE_WINDOW_API_DISCONNECT            = 14,   
    NATIVE_WINDOW_SET_BUFFERS_USER_DIMENSIONS = 15, 
    NATIVE_WINDOW_SET_POST_TRANSFORM_CROP   = 16,   
};


enum {
    


    NATIVE_WINDOW_API_EGL = 1,

    

    NATIVE_WINDOW_API_CPU = 2,

    


    NATIVE_WINDOW_API_MEDIA = 3,

    

    NATIVE_WINDOW_API_CAMERA = 4,
};


enum {
    
    NATIVE_WINDOW_TRANSFORM_FLIP_H = HAL_TRANSFORM_FLIP_H ,
    
    NATIVE_WINDOW_TRANSFORM_FLIP_V = HAL_TRANSFORM_FLIP_V,
    
    NATIVE_WINDOW_TRANSFORM_ROT_90 = HAL_TRANSFORM_ROT_90,
    
    NATIVE_WINDOW_TRANSFORM_ROT_180 = HAL_TRANSFORM_ROT_180,
    
    NATIVE_WINDOW_TRANSFORM_ROT_270 = HAL_TRANSFORM_ROT_270,
};


enum {
    


    NATIVE_WINDOW_SCALING_MODE_FREEZE           = 0,
    
    NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW  = 1,
    


    NATIVE_WINDOW_SCALING_MODE_SCALE_CROP       = 2,
    



    NATIVE_WINDOW_SCALING_MODE_NO_SCALE_CROP    = 3,
};


enum {
    NATIVE_WINDOW_FRAMEBUFFER               = 0, 
    NATIVE_WINDOW_SURFACE                   = 1, 
};







static const int64_t NATIVE_WINDOW_TIMESTAMP_AUTO = (-9223372036854775807LL-1);

struct ANativeWindow
{
#ifdef __cplusplus
    ANativeWindow()
        : flags(0), minSwapInterval(0), maxSwapInterval(0), xdpi(0), ydpi(0)
    {
        common.magic = ANDROID_NATIVE_WINDOW_MAGIC;
        common.version = sizeof(ANativeWindow);
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

    
    const uint32_t flags;

    
    const int   minSwapInterval;

    
    const int   maxSwapInterval;

    
    const float xdpi;
    const float ydpi;

    
    intptr_t    oem[4];

    




    int     (*setSwapInterval)(struct ANativeWindow* window,
                int interval);

    
















    int     (*dequeueBuffer_DEPRECATED)(struct ANativeWindow* window,
                struct ANativeWindowBuffer** buffer);

    










    int     (*lockBuffer_DEPRECATED)(struct ANativeWindow* window,
                struct ANativeWindowBuffer* buffer);

    


















    int     (*queueBuffer_DEPRECATED)(struct ANativeWindow* window,
                struct ANativeWindowBuffer* buffer);

    




    int     (*query)(const struct ANativeWindow* window,
                int what, int* value);

    































    int     (*perform)(struct ANativeWindow* window,
                int operation, ... );

    
















    int     (*cancelBuffer_DEPRECATED)(struct ANativeWindow* window,
                struct ANativeWindowBuffer* buffer);

    


















    int     (*dequeueBuffer)(struct ANativeWindow* window,
                struct ANativeWindowBuffer** buffer, int* fenceFd);

    


















    int     (*queueBuffer)(struct ANativeWindow* window,
                struct ANativeWindowBuffer* buffer, int fenceFd);

    
























    int     (*cancelBuffer)(struct ANativeWindow* window,
                struct ANativeWindowBuffer* buffer, int fenceFd);
};

 


typedef struct ANativeWindow ANativeWindow;
typedef struct ANativeWindow android_native_window_t;












static inline int native_window_set_usage(
        struct ANativeWindow* window, int usage)
{
    return window->perform(window, NATIVE_WINDOW_SET_USAGE, usage);
}


static inline int native_window_connect(
        struct ANativeWindow* window, int api) {
    return 0;
}


static inline int native_window_disconnect(
        struct ANativeWindow* window, int api) {
    return 0;
}















static inline int native_window_set_crop(
        struct ANativeWindow* window,
        android_native_rect_t const * crop)
{
    return window->perform(window, NATIVE_WINDOW_SET_CROP, crop);
}















static inline int native_window_set_post_transform_crop(
        struct ANativeWindow* window,
        android_native_rect_t const * crop)
{
    return window->perform(window, NATIVE_WINDOW_SET_POST_TRANSFORM_CROP, crop);
}








static inline int native_window_set_active_rect(
        struct ANativeWindow* window,
        android_native_rect_t const * active_rect)
{
    return native_window_set_post_transform_crop(window, active_rect);
}





static inline int native_window_set_buffer_count(
        struct ANativeWindow* window,
        size_t bufferCount)
{
    return window->perform(window, NATIVE_WINDOW_SET_BUFFER_COUNT, bufferCount);
}










static inline int native_window_set_buffers_geometry(
        struct ANativeWindow* window,
        int w, int h, int format)
{
    return window->perform(window, NATIVE_WINDOW_SET_BUFFERS_GEOMETRY,
            w, h, format);
}














static inline int native_window_set_buffers_dimensions(
        struct ANativeWindow* window,
        int w, int h)
{
    return window->perform(window, NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS,
            w, h);
}


















static inline int native_window_set_buffers_user_dimensions(
        struct ANativeWindow* window,
        int w, int h)
{
    return window->perform(window, NATIVE_WINDOW_SET_BUFFERS_USER_DIMENSIONS,
            w, h);
}







static inline int native_window_set_buffers_format(
        struct ANativeWindow* window,
        int format)
{
    return window->perform(window, NATIVE_WINDOW_SET_BUFFERS_FORMAT, format);
}






static inline int native_window_set_buffers_transform(
        struct ANativeWindow* window,
        int transform)
{
    return window->perform(window, NATIVE_WINDOW_SET_BUFFERS_TRANSFORM,
            transform);
}











static inline int native_window_set_buffers_timestamp(
        struct ANativeWindow* window,
        int64_t timestamp)
{
    return window->perform(window, NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP,
            timestamp);
}






static inline int native_window_set_scaling_mode(
        struct ANativeWindow* window,
        int mode)
{
    return window->perform(window, NATIVE_WINDOW_SET_SCALING_MODE,
            mode);
}







static inline int native_window_api_connect(
        struct ANativeWindow* window, int api)
{
    return window->perform(window, NATIVE_WINDOW_API_CONNECT, api);
}







static inline int native_window_api_disconnect(
        struct ANativeWindow* window, int api)
{
    return window->perform(window, NATIVE_WINDOW_API_DISCONNECT, api);
}







static inline int native_window_dequeue_buffer_and_wait(ANativeWindow *anw,
        struct ANativeWindowBuffer** anb) {
    return anw->dequeueBuffer_DEPRECATED(anw, anb);
}


__END_DECLS

#endif 
