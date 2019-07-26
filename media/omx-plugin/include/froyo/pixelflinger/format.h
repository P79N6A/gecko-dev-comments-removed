















#ifndef ANDROID_PIXELFLINGER_FORMAT_H
#define ANDROID_PIXELFLINGER_FORMAT_H

#include <stdint.h>
#include <sys/types.h>

enum GGLPixelFormat {
    
    
    GGL_PIXEL_FORMAT_UNKNOWN    =   0,
    GGL_PIXEL_FORMAT_NONE       =   0,

    GGL_PIXEL_FORMAT_RGBA_8888   =   1,  
    GGL_PIXEL_FORMAT_RGBX_8888   =   2,  
    GGL_PIXEL_FORMAT_RGB_888     =   3,  
    GGL_PIXEL_FORMAT_RGB_565     =   4,  
    GGL_PIXEL_FORMAT_BGRA_8888   =   5,  
    GGL_PIXEL_FORMAT_RGBA_5551   =   6,  
    GGL_PIXEL_FORMAT_RGBA_4444   =   7,  

    GGL_PIXEL_FORMAT_A_8         =   8,  
    GGL_PIXEL_FORMAT_L_8         =   9,  
    GGL_PIXEL_FORMAT_LA_88       = 0xA,  
    GGL_PIXEL_FORMAT_RGB_332     = 0xB,  

    
    GGL_PIXEL_FORMAT_RESERVED_10 = 0x10,
    GGL_PIXEL_FORMAT_RESERVED_11 = 0x11,
    GGL_PIXEL_FORMAT_RESERVED_12 = 0x12,
    GGL_PIXEL_FORMAT_RESERVED_13 = 0x13,
    GGL_PIXEL_FORMAT_RESERVED_14 = 0x14,
    GGL_PIXEL_FORMAT_RESERVED_15 = 0x15,
    GGL_PIXEL_FORMAT_RESERVED_16 = 0x16,
    GGL_PIXEL_FORMAT_RESERVED_17 = 0x17,

    
    GGL_PIXEL_FORMAT_Z_16       =  0x18,
    GGL_PIXEL_FORMAT_S_8        =  0x19,
    GGL_PIXEL_FORMAT_SZ_24      =  0x1A,
    GGL_PIXEL_FORMAT_SZ_8       =  0x1B,

    
    GGL_PIXEL_FORMAT_RESERVED_20 = 0x20,
    GGL_PIXEL_FORMAT_RESERVED_21 = 0x21,
};

enum GGLFormatComponents {
	GGL_STENCIL_INDEX		= 0x1901,
	GGL_DEPTH_COMPONENT		= 0x1902,
	GGL_ALPHA				= 0x1906,
	GGL_RGB					= 0x1907,
	GGL_RGBA				= 0x1908,
	GGL_LUMINANCE			= 0x1909,
	GGL_LUMINANCE_ALPHA		= 0x190A,
};

enum GGLFormatComponentIndex {
    GGL_INDEX_ALPHA   = 0,
    GGL_INDEX_RED     = 1,
    GGL_INDEX_GREEN   = 2,
    GGL_INDEX_BLUE    = 3,
    GGL_INDEX_STENCIL = 0,
    GGL_INDEX_DEPTH   = 1,
    GGL_INDEX_Y       = 0,
    GGL_INDEX_CB      = 1,
    GGL_INDEX_CR      = 2,
};

typedef struct {
#ifdef __cplusplus
    enum {
        ALPHA   = GGL_INDEX_ALPHA,
        RED     = GGL_INDEX_RED,
        GREEN   = GGL_INDEX_GREEN,
        BLUE    = GGL_INDEX_BLUE,
        STENCIL = GGL_INDEX_STENCIL,
        DEPTH   = GGL_INDEX_DEPTH,
        LUMA    = GGL_INDEX_Y,
        CHROMAB = GGL_INDEX_CB,
        CHROMAR = GGL_INDEX_CR,
    };
    inline uint32_t mask(int i) const {
            return ((1<<(c[i].h-c[i].l))-1)<<c[i].l;
    }
    inline uint32_t bits(int i) const {
            return c[i].h - c[i].l;
    }
#endif
	uint8_t     size;	
    uint8_t     bitsPerPixel;
    union {    
        struct {
            uint8_t     ah;		
            uint8_t     al;		
            uint8_t     rh;		
            uint8_t     rl;		
            uint8_t     gh;		
            uint8_t     gl;		
            uint8_t     bh;		
            uint8_t     bl;		
        };
        struct {
            uint8_t h;
            uint8_t l;
        } __attribute__((__packed__)) c[4];        
    } __attribute__((__packed__));
	uint16_t    components;	
} GGLFormat;


#ifdef __cplusplus
extern "C" const GGLFormat* gglGetPixelFormatTable(size_t* numEntries = 0);
#else
const GGLFormat* gglGetPixelFormatTable(size_t* numEntries);
#endif




#endif 
