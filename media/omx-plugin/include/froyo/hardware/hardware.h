















#ifndef ANDROID_INCLUDE_HARDWARE_HARDWARE_H
#define ANDROID_INCLUDE_HARDWARE_HARDWARE_H

#include <stdint.h>
#include <sys/cdefs.h>

#include <cutils/native_handle.h>

__BEGIN_DECLS





#define MAKE_TAG_CONSTANT(A,B,C,D) (((A) << 24) | ((B) << 16) | ((C) << 8) | (D))

#define HARDWARE_MODULE_TAG MAKE_TAG_CONSTANT('H', 'W', 'M', 'T')
#define HARDWARE_DEVICE_TAG MAKE_TAG_CONSTANT('H', 'W', 'D', 'T')

struct hw_module_t;
struct hw_module_methods_t;
struct hw_device_t;






typedef struct hw_module_t {
    
    uint32_t tag;

    
    uint16_t version_major;

    
    uint16_t version_minor;

    
    const char *id;

    
    const char *name;

    
    const char *author;

    
    struct hw_module_methods_t* methods;

    
    void* dso;

    
    uint32_t reserved[32-7];

} hw_module_t;

typedef struct hw_module_methods_t {
    
    int (*open)(const struct hw_module_t* module, const char* id,
            struct hw_device_t** device);

} hw_module_methods_t;





typedef struct hw_device_t {
    
    uint32_t tag;

    
    uint32_t version;

    
    struct hw_module_t* module;

    
    uint32_t reserved[12];

    
    int (*close)(struct hw_device_t* device);

} hw_device_t;




#define HAL_MODULE_INFO_SYM         HMI




#define HAL_MODULE_INFO_SYM_AS_STR  "HMI"





int hw_get_module(const char *id, const struct hw_module_t **module);






enum {
    HAL_PIXEL_FORMAT_RGBA_8888    = 1,
    HAL_PIXEL_FORMAT_RGBX_8888    = 2,
    HAL_PIXEL_FORMAT_RGB_888      = 3,
    HAL_PIXEL_FORMAT_RGB_565      = 4,
    HAL_PIXEL_FORMAT_BGRA_8888    = 5,
    HAL_PIXEL_FORMAT_RGBA_5551    = 6,
    HAL_PIXEL_FORMAT_RGBA_4444    = 7,
    HAL_PIXEL_FORMAT_YCbCr_422_SP = 0x10,
    HAL_PIXEL_FORMAT_YCrCb_420_SP = 0x11,
    HAL_PIXEL_FORMAT_YCbCr_422_P  = 0x12,
    HAL_PIXEL_FORMAT_YCbCr_420_P  = 0x13,
    HAL_PIXEL_FORMAT_YCbCr_422_I  = 0x14,
    HAL_PIXEL_FORMAT_YCbCr_420_I  = 0x15,
    HAL_PIXEL_FORMAT_CbYCrY_422_I = 0x16,
    HAL_PIXEL_FORMAT_CbYCrY_420_I = 0x17,
    HAL_PIXEL_FORMAT_YCbCr_420_SP_TILED = 0x20,
    HAL_PIXEL_FORMAT_YCbCr_420_SP       = 0x21,
    HAL_PIXEL_FORMAT_YCrCb_420_SP_TILED = 0x22,
    HAL_PIXEL_FORMAT_YCrCb_422_SP       = 0x23,
};






enum {
    
    HAL_TRANSFORM_FLIP_H    = 0x01,
    
    HAL_TRANSFORM_FLIP_V    = 0x02,
    
    HAL_TRANSFORM_ROT_90    = 0x04,
    
    HAL_TRANSFORM_ROT_180   = 0x03,
    
    HAL_TRANSFORM_ROT_270   = 0x07,
};

__END_DECLS

#endif  
