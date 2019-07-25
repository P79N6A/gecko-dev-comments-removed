




#ifndef GFX_COLOR_H
#define GFX_COLOR_H

#include "gfxTypes.h"

#include "prbit.h" 
#include "prio.h"  

#include "mozilla/Attributes.h" 

#define GFX_UINT32_FROM_BPTR(pbptr,i) (((uint32_t*)(pbptr))[i])

#if defined(IS_BIG_ENDIAN)
  #define GFX_NTOHL(x) (x)
  #define GFX_HAVE_CHEAP_NTOHL
#elif defined(_WIN32)
  #if (_MSC_VER >= 1300) 
    #include <stdlib.h>
    #pragma intrinsic(_byteswap_ulong)
    #define GFX_NTOHL(x) _byteswap_ulong(x)
    #define GFX_HAVE_CHEAP_NTOHL
  #else
    
    #define GFX_NTOHL(x) \
         ( (PR_ROTATE_RIGHT32((x),8) & 0xFF00FF00) | \
           (PR_ROTATE_LEFT32((x),8)  & 0x00FF00FF) )
  #endif
#else
  #define GFX_NTOHL(x) ntohl(x)
  #define GFX_HAVE_CHEAP_NTOHL
#endif











#if defined(GFX_HAVE_CHEAP_NTOHL)
  #define GFX_0XFF_PPIXEL_FROM_UINT32(x) \
       ( (GFX_NTOHL(x) >> 8) | (0xFF << 24) )
#else
  
  #define GFX_0XFF_PPIXEL_FROM_UINT32(x) \
       ( (PR_ROTATE_LEFT32((x),16) | 0xFF00FF00) & ((x) | 0xFFFF00FF) )
#endif

#define GFX_0XFF_PPIXEL_FROM_BPTR(x) \
     ( GFX_0XFF_PPIXEL_FROM_UINT32(GFX_UINT32_FROM_BPTR((x),0)) )









#define GFX_BLOCK_RGB_TO_FRGB(from,to) \
  PR_BEGIN_MACRO \
    uint32_t m0 = GFX_UINT32_FROM_BPTR(from,0), \
             m1 = GFX_UINT32_FROM_BPTR(from,1), \
             m2 = GFX_UINT32_FROM_BPTR(from,2), \
             rgbr = GFX_NTOHL(m0), \
             gbrg = GFX_NTOHL(m1), \
             brgb = GFX_NTOHL(m2), \
             p0, p1, p2, p3; \
    p0 = 0xFF000000 | ((rgbr) >>  8); \
    p1 = 0xFF000000 | ((rgbr) << 16) | ((gbrg) >> 16); \
    p2 = 0xFF000000 | ((gbrg) <<  8) | ((brgb) >> 24); \
    p3 = 0xFF000000 | (brgb); \
    to[0] = p0; to[1] = p1; to[2] = p2; to[3] = p3; \
  PR_END_MACRO









#define GFX_DIVIDE_BY_255(v)  \
     (((((unsigned)(v)) << 8) + ((unsigned)(v)) + 255) >> 16)






PRUint8 MOZ_ALWAYS_INLINE gfxPreMultiply(PRUint8 c, PRUint8 a) {
    return GFX_DIVIDE_BY_255((c)*(a));
}





PRUint32 MOZ_ALWAYS_INLINE
gfxPackedPixelNoPreMultiply(PRUint8 a, PRUint8 r, PRUint8 g, PRUint8 b) {
    return (((a) << 24) | ((r) << 16) | ((g) << 8) | (b));
}





PRUint32 MOZ_ALWAYS_INLINE
gfxPackedPixel(PRUint8 a, PRUint8 r, PRUint8 g, PRUint8 b) {
    if (a == 0x00)
        return 0x00000000;
    else if (a == 0xFF) {
        return gfxPackedPixelNoPreMultiply(a, r, g, b);
    } else {
        return  ((a) << 24) |
                (gfxPreMultiply(r,a) << 16) |
                (gfxPreMultiply(g,a) << 8)  |
                (gfxPreMultiply(b,a));
    }
}








struct THEBES_API gfxRGBA {
    gfxFloat r, g, b, a;

    enum PackedColorType {
        PACKED_ABGR,
        PACKED_ABGR_PREMULTIPLIED,

        PACKED_ARGB,
        PACKED_ARGB_PREMULTIPLIED,

        PACKED_XRGB
    };

    gfxRGBA() { }
    



    gfxRGBA(gfxFloat _r, gfxFloat _g, gfxFloat _b, gfxFloat _a=1.0) : r(_r), g(_g), b(_b), a(_a) {}

    








    gfxRGBA(uint32_t c, PackedColorType colorType = PACKED_ABGR) {
        if (colorType == PACKED_ABGR ||
            colorType == PACKED_ABGR_PREMULTIPLIED)
        {
            r = ((c >> 0) & 0xff) * (1.0 / 255.0);
            g = ((c >> 8) & 0xff) * (1.0 / 255.0);
            b = ((c >> 16) & 0xff) * (1.0 / 255.0);
            a = ((c >> 24) & 0xff) * (1.0 / 255.0);
        } else if (colorType == PACKED_ARGB ||
                   colorType == PACKED_XRGB ||
                   colorType == PACKED_ARGB_PREMULTIPLIED)
        {
            b = ((c >> 0) & 0xff) * (1.0 / 255.0);
            g = ((c >> 8) & 0xff) * (1.0 / 255.0);
            r = ((c >> 16) & 0xff) * (1.0 / 255.0);
            a = ((c >> 24) & 0xff) * (1.0 / 255.0);
        }

        if (colorType == PACKED_ABGR_PREMULTIPLIED ||
            colorType == PACKED_ARGB_PREMULTIPLIED)
        {
            if (a > 0.0) {
                r /= a;
                g /= a;
                b /= a;
            }
        } else if (colorType == PACKED_XRGB) {
            a = 1.0;
        }
    }

    bool operator==(const gfxRGBA& other) const
    {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
    bool operator!=(const gfxRGBA& other) const
    {
        return !(*this == other);
    }

    







    uint32_t Packed(PackedColorType colorType = PACKED_ABGR) const {
        gfxFloat rb = (r * 255.0);
        gfxFloat gb = (g * 255.0);
        gfxFloat bb = (b * 255.0);
        gfxFloat ab = (a * 255.0);

        if (colorType == PACKED_ABGR) {
            return (uint8_t(ab) << 24) |
                   (uint8_t(bb) << 16) |
                   (uint8_t(gb) << 8) |
                   (uint8_t(rb) << 0);
        }
        if (colorType == PACKED_ARGB || colorType == PACKED_XRGB) {
            return (uint8_t(ab) << 24) |
                   (uint8_t(rb) << 16) |
                   (uint8_t(gb) << 8) |
                   (uint8_t(bb) << 0);
        }

        rb *= a;
        gb *= a;
        bb *= a;

        if (colorType == PACKED_ABGR_PREMULTIPLIED) {
            return (((uint8_t)(ab) << 24) |
                    ((uint8_t)(bb) << 16) |
                    ((uint8_t)(gb) << 8) |
                    ((uint8_t)(rb) << 0));
        }
        if (colorType == PACKED_ARGB_PREMULTIPLIED) {
            return (((uint8_t)(ab) << 24) |
                    ((uint8_t)(rb) << 16) |
                    ((uint8_t)(gb) << 8) |
                    ((uint8_t)(bb) << 0));
        }

        return 0;
    }
};

#endif 
