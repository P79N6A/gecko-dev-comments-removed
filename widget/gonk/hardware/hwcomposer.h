















#ifndef ANDROID_INCLUDE_HARDWARE_HWCOMPOSER_H
#define ANDROID_INCLUDE_HARDWARE_HWCOMPOSER_H

#include <stdint.h>
#include <sys/cdefs.h>

#include <hardware/gralloc.h>
#include <hardware/hardware.h>
#include <cutils/native_handle.h>

__BEGIN_DECLS



#define HWC_API_VERSION 1




#define HWC_HARDWARE_MODULE_ID "hwcomposer"




#define HWC_HARDWARE_COMPOSER   "composer"


enum {
    
    HWC_EGL_ERROR = -1
};





enum {
    






    HWC_HINT_TRIPLE_BUFFER  = 0x00000001,

    





    HWC_HINT_CLEAR_FB       = 0x00000002
};





enum {
    




    HWC_SKIP_LAYER = 0x00000001,
};




enum {
    
    HWC_FRAMEBUFFER = 0,

    
    HWC_OVERLAY = 1,
};




enum {
    
    HWC_BLENDING_NONE     = 0x0100,

    
    HWC_BLENDING_PREMULT  = 0x0105,

    
    HWC_BLENDING_COVERAGE = 0x0405
};




enum {
    
    HWC_TRANSFORM_FLIP_H = HAL_TRANSFORM_FLIP_H,
    
    HWC_TRANSFORM_FLIP_V = HAL_TRANSFORM_FLIP_V,
    
    HWC_TRANSFORM_ROT_90 = HAL_TRANSFORM_ROT_90,
    
    HWC_TRANSFORM_ROT_180 = HAL_TRANSFORM_ROT_180,
    
    HWC_TRANSFORM_ROT_270 = HAL_TRANSFORM_ROT_270,
};

typedef struct hwc_rect {
    int left;
    int top;
    int right;
    int bottom;
} hwc_rect_t;

typedef struct hwc_region {
    size_t numRects;
    hwc_rect_t const* rects;
} hwc_region_t;

typedef struct hwc_layer {
    





    int32_t compositionType;

    
    uint32_t hints;

    
    uint32_t flags;

    

    buffer_handle_t handle;

    
    uint32_t transform;

    
    int32_t blending;

    

    hwc_rect_t sourceCrop;

    



    hwc_rect_t displayFrame;

    



    hwc_region_t visibleRegionScreen;
} hwc_layer_t;





enum {
    



    HWC_GEOMETRY_CHANGED = 0x00000001,
};





typedef struct hwc_layer_list {
    uint32_t flags;
    size_t numHwLayers;
    hwc_layer_t hwLayers[0];
} hwc_layer_list_t;


typedef void* hwc_display_t;


typedef void* hwc_surface_t;






typedef struct hwc_procs {
    








    void (*invalidate)(struct hwc_procs* procs);
} hwc_procs_t;




typedef struct hwc_module {
    struct hw_module_t common;
} hwc_module_t;


typedef struct hwc_composer_device {
    struct hw_device_t common;

    






















    int (*prepare)(struct hwc_composer_device *dev, hwc_layer_list_t* list);


    









































    int (*set)(struct hwc_composer_device *dev,
                hwc_display_t dpy,
                hwc_surface_t sur,
                hwc_layer_list_t* list);
    




    void (*dump)(struct hwc_composer_device* dev, char *buff, int buff_len);

    












    void (*registerProcs)(struct hwc_composer_device* dev,
            hwc_procs_t const* procs);

    void* reserved_proc[6];

} hwc_composer_device_t;




static inline int hwc_open(const struct hw_module_t* module,
        hwc_composer_device_t** device) {
    return module->methods->open(module,
            HWC_HARDWARE_COMPOSER, (struct hw_device_t**)device);
}

static inline int hwc_close(hwc_composer_device_t* device) {
    return device->common.close(&device->common);
}




__END_DECLS

#endif 
