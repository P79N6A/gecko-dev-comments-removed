























#ifndef UI_PIXELFORMAT_H
#define UI_PIXELFORMAT_H

#include <stdint.h>
#include <sys/types.h>
#include <utils/Errors.h>
#include <hardware/hardware.h>

namespace android {

enum {
    
    
    
    
    PIXEL_FORMAT_UNKNOWN    =   0,
    PIXEL_FORMAT_NONE       =   0,

    
    PIXEL_FORMAT_CUSTOM         = -4,
        

    PIXEL_FORMAT_TRANSLUCENT    = -3,
        

    PIXEL_FORMAT_TRANSPARENT    = -2,
        
        

    PIXEL_FORMAT_OPAQUE         = -1,
        

    

    PIXEL_FORMAT_RGBA_8888   = HAL_PIXEL_FORMAT_RGBA_8888,  
    PIXEL_FORMAT_RGBX_8888   = HAL_PIXEL_FORMAT_RGBX_8888,  
    PIXEL_FORMAT_RGB_888     = HAL_PIXEL_FORMAT_RGB_888,    
    PIXEL_FORMAT_RGB_565     = HAL_PIXEL_FORMAT_RGB_565,    
    PIXEL_FORMAT_BGRA_8888   = HAL_PIXEL_FORMAT_BGRA_8888,  
    PIXEL_FORMAT_RGBA_5551   = HAL_PIXEL_FORMAT_RGBA_5551,  
    PIXEL_FORMAT_RGBA_4444   = HAL_PIXEL_FORMAT_RGBA_4444,  
    PIXEL_FORMAT_A_8         = 8,                           
};

typedef int32_t PixelFormat;

struct PixelFormatInfo {
    enum {
        INDEX_ALPHA   = 0,
        INDEX_RED     = 1,
        INDEX_GREEN   = 2,
        INDEX_BLUE    = 3
    };

    enum { 
        ALPHA   = 1,
        RGB     = 2,
        RGBA    = 3,
        L       = 4,
        LA      = 5,
        OTHER   = 0xFF
    };

    struct szinfo {
        uint8_t h;
        uint8_t l;
    };

    inline PixelFormatInfo() : version(sizeof(PixelFormatInfo)) { }
    size_t getScanlineSize(unsigned int width) const;
    size_t getSize(size_t ci) const {
        return (ci <= 3) ? (cinfo[ci].h - cinfo[ci].l) : 0;
    }
    size_t      version;
    PixelFormat format;
    size_t      bytesPerPixel;
    size_t      bitsPerPixel;
    union {
        szinfo      cinfo[4];
        struct {
            uint8_t     h_alpha;
            uint8_t     l_alpha;
            uint8_t     h_red;
            uint8_t     l_red;
            uint8_t     h_green;
            uint8_t     l_green;
            uint8_t     h_blue;
            uint8_t     l_blue;
        };
    };
    uint8_t     components;
    uint8_t     reserved0[3];
    uint32_t    reserved1;
};



ssize_t     bytesPerPixel(PixelFormat format);
ssize_t     bitsPerPixel(PixelFormat format);
status_t    getPixelFormatInfo(PixelFormat format, PixelFormatInfo* info);

}; 

#endif 
