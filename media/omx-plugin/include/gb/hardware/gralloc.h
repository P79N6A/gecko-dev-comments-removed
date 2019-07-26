
















#ifndef ANDROID_GRALLOC_INTERFACE_H
#define ANDROID_GRALLOC_INTERFACE_H

#include <cutils/native_handle.h>

#include <hardware/hardware.h>

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

__BEGIN_DECLS




#define GRALLOC_HARDWARE_MODULE_ID "gralloc"





#define GRALLOC_HARDWARE_FB0 "fb0"
#define GRALLOC_HARDWARE_GPU0 "gpu0"

enum {
    
    GRALLOC_USAGE_SW_READ_NEVER   = 0x00000000,
    
    GRALLOC_USAGE_SW_READ_RARELY  = 0x00000002,
    
    GRALLOC_USAGE_SW_READ_OFTEN   = 0x00000003,
    
    GRALLOC_USAGE_SW_READ_MASK    = 0x0000000F,
    
    
    GRALLOC_USAGE_SW_WRITE_NEVER  = 0x00000000,
    
    GRALLOC_USAGE_SW_WRITE_RARELY = 0x00000020,
    
    GRALLOC_USAGE_SW_WRITE_OFTEN  = 0x00000030,
    
    GRALLOC_USAGE_SW_WRITE_MASK   = 0x000000F0,

    
    GRALLOC_USAGE_HW_TEXTURE      = 0x00000100,
    
    GRALLOC_USAGE_HW_RENDER       = 0x00000200,
    
    GRALLOC_USAGE_HW_2D           = 0x00000400,
    
    GRALLOC_USAGE_HW_FB           = 0x00001000,
    
    GRALLOC_USAGE_HW_MASK         = 0x00001F00,

    
    GRALLOC_USAGE_PRIVATE_0       = 0x10000000,
    GRALLOC_USAGE_PRIVATE_1       = 0x20000000,
    GRALLOC_USAGE_PRIVATE_2       = 0x40000000,
    GRALLOC_USAGE_PRIVATE_3       = 0x80000000,
    GRALLOC_USAGE_PRIVATE_MASK    = 0xF0000000,
};



typedef const native_handle* buffer_handle_t;

enum {
    



    GRALLOC_MODULE_PERFORM_CREATE_HANDLE_FROM_BUFFER = 0x080000001,
};






typedef struct gralloc_module_t {
    struct hw_module_t common;
    
    














    int (*registerBuffer)(struct gralloc_module_t const* module,
            buffer_handle_t handle);

    











    int (*unregisterBuffer)(struct gralloc_module_t const* module,
            buffer_handle_t handle);
    
    





























    
    int (*lock)(struct gralloc_module_t const* module,
            buffer_handle_t handle, int usage,
            int l, int t, int w, int h,
            void** vaddr);

    
    



    
    int (*unlock)(struct gralloc_module_t const* module,
            buffer_handle_t handle);


    
    int (*perform)(struct gralloc_module_t const* module,
            int operation, ... );

    
    void* reserved_proc[7];
} gralloc_module_t;








typedef struct alloc_device_t {
    struct hw_device_t common;

    








    
    int (*alloc)(struct alloc_device_t* dev,
            int w, int h, int format, int usage,
            buffer_handle_t* handle, int* stride);

    









    int (*free)(struct alloc_device_t* dev,
            buffer_handle_t handle);

} alloc_device_t;


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

    int reserved[8];
    
    




    int (*setSwapInterval)(struct framebuffer_device_t* window,
            int interval);

    



















    int (*setUpdateRect)(struct framebuffer_device_t* window,
            int left, int top, int width, int height);
    
    


















    int (*post)(struct framebuffer_device_t* dev, buffer_handle_t buffer);


    




    int (*compositionComplete)(struct framebuffer_device_t* dev);


    void* reserved_proc[8];

} framebuffer_device_t;




static inline int gralloc_open(const struct hw_module_t* module, 
        struct alloc_device_t** device) {
    return module->methods->open(module, 
            GRALLOC_HARDWARE_GPU0, (struct hw_device_t**)device);
}

static inline int gralloc_close(struct alloc_device_t* device) {
    return device->common.close(&device->common);
}


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
