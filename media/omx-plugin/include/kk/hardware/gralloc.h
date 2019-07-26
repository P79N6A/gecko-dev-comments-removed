
















#ifndef ANDROID_GRALLOC_INTERFACE_H
#define ANDROID_GRALLOC_INTERFACE_H

#include <system/window.h>
#include <system/graphics.h>
#include <hardware/hardware.h>

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <cutils/native_handle.h>

#include <hardware/hardware.h>
#include <hardware/fb.h>

__BEGIN_DECLS














#define GRALLOC_MODULE_API_VERSION_0_1  HARDWARE_MODULE_API_VERSION(0, 1)
#define GRALLOC_MODULE_API_VERSION_0_2  HARDWARE_MODULE_API_VERSION(0, 2)

#define GRALLOC_DEVICE_API_VERSION_0_1  HARDWARE_DEVICE_API_VERSION(0, 1)




#define GRALLOC_HARDWARE_MODULE_ID "gralloc"





#define GRALLOC_HARDWARE_GPU0 "gpu0"

enum {
    
    GRALLOC_USAGE_SW_READ_NEVER         = 0x00000000,
    
    GRALLOC_USAGE_SW_READ_RARELY        = 0x00000002,
    
    GRALLOC_USAGE_SW_READ_OFTEN         = 0x00000003,
    
    GRALLOC_USAGE_SW_READ_MASK          = 0x0000000F,
    
    
    GRALLOC_USAGE_SW_WRITE_NEVER        = 0x00000000,
    
    GRALLOC_USAGE_SW_WRITE_RARELY       = 0x00000020,
    
    GRALLOC_USAGE_SW_WRITE_OFTEN        = 0x00000030,
    
    GRALLOC_USAGE_SW_WRITE_MASK         = 0x000000F0,

    
    GRALLOC_USAGE_HW_TEXTURE            = 0x00000100,
    
    GRALLOC_USAGE_HW_RENDER             = 0x00000200,
    
    GRALLOC_USAGE_HW_2D                 = 0x00000400,
    
    GRALLOC_USAGE_HW_COMPOSER           = 0x00000800,
    
    GRALLOC_USAGE_HW_FB                 = 0x00001000,
    
    GRALLOC_USAGE_HW_VIDEO_ENCODER      = 0x00010000,
    
    GRALLOC_USAGE_HW_CAMERA_WRITE       = 0x00020000,
    
    GRALLOC_USAGE_HW_CAMERA_READ        = 0x00040000,
    
    GRALLOC_USAGE_HW_CAMERA_ZSL         = 0x00060000,
    
    GRALLOC_USAGE_HW_CAMERA_MASK        = 0x00060000,
    
    GRALLOC_USAGE_HW_MASK               = 0x00071F00,

    


    GRALLOC_USAGE_EXTERNAL_DISP         = 0x00002000,

    





    GRALLOC_USAGE_PROTECTED             = 0x00004000,

    
    
    GRALLOC_USAGE_MDS_SESSION_ID_MASK   = 0x0F000000,

    
    GRALLOC_USAGE_PRIVATE_0             = 0x10000000,
    GRALLOC_USAGE_PRIVATE_1             = 0x20000000,
    GRALLOC_USAGE_PRIVATE_2             = 0x40000000,
    
    GRALLOC_USAGE_PRIVATE_3             = 0x80000000,
    GRALLOC_USAGE_PRIVATE_MASK          = 0xF0000000,
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

    










    int (*lock_ycbcr)(struct gralloc_module_t const* module,
            buffer_handle_t handle, int usage,
            int l, int t, int w, int h,
            struct android_ycbcr *ycbcr);

    
    void* reserved_proc[6];
} gralloc_module_t;








typedef struct alloc_device_t {
    struct hw_device_t common;

    












    
    int (*alloc)(struct alloc_device_t* dev,
            int w, int h, int format, int usage,
            buffer_handle_t* handle, int* stride);

    









    int (*free)(struct alloc_device_t* dev,
            buffer_handle_t handle);

    



    void (*dump)(struct alloc_device_t *dev, char *buff, int buff_len);

    void* reserved_proc[7];
} alloc_device_t;




static inline int gralloc_open(const struct hw_module_t* module, 
        struct alloc_device_t** device) {
    return module->methods->open(module, 
            GRALLOC_HARDWARE_GPU0, (struct hw_device_t**)device);
}

static inline int gralloc_close(struct alloc_device_t* device) {
    return device->common.close(&device->common);
}

__END_DECLS

#endif  
