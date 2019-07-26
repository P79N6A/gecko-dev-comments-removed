
















#ifndef ANDROID_FB_INTERFACE_H
#define ANDROID_FB_INTERFACE_H

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <cutils/native_handle.h>

#include <hardware/hardware.h>

__BEGIN_DECLS

#define GRALLOC_HARDWARE_FB0 "fb0"






typedef struct framebuffer_device_t {
    struct hw_device_t common;

    
    const uint32_t  flags;

    
    const uint32_t  width;
    const uint32_t  height;

    
    const int       stride;

    
    const int       format;

    
    const float     xdpi;
    const float     ydpi;

    
    const float     fps;

    
    const int       minSwapInterval;

    
    const int       maxSwapInterval;

    
    const int       numFramebuffers;

    int reserved[7];

    




    int (*setSwapInterval)(struct framebuffer_device_t* window,
            int interval);

    



















    int (*setUpdateRect)(struct framebuffer_device_t* window,
            int left, int top, int width, int height);

    


















    int (*post)(struct framebuffer_device_t* dev, buffer_handle_t buffer);


    




    int (*compositionComplete)(struct framebuffer_device_t* dev);

    




    void (*dump)(struct framebuffer_device_t* dev, char *buff, int buff_len);

    





    int (*enableScreen)(struct framebuffer_device_t* dev, int enable);
    int (*setFramecount)(int cmd, int count, int x, int y);
    void* reserved_proc[6];

} framebuffer_device_t;




static inline int framebuffer_open(const struct hw_module_t* module,
        struct framebuffer_device_t** device) {
    return module->methods->open(module,
            GRALLOC_HARDWARE_FB0, (struct hw_device_t**)device);
}

static inline int framebuffer_close(struct framebuffer_device_t* device) {
    return device->common.close(&device->common);
}


__END_DECLS

#endif  
