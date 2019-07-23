




































#ifndef GFX_COLOR_H
#define GFX_COLOR_H

#ifdef MOZILLA_INTERNAL_API
#include "nsPrintfCString.h"
#endif

#include "gfxTypes.h"

#include "prbit.h" 
#include "prio.h"  

#define GFX_UINT32_FROM_BPTR(pbptr,i) (((PRUint32*)(pbptr))[i])

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
    PRUint32 m0 = GFX_UINT32_FROM_BPTR(from,0), \
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






#define GFX_PREMULTIPLY(c,a) GFX_DIVIDE_BY_255((c)*(a))








#define GFX_PACKED_PIXEL(a,r,g,b)                                       \
    ((a) == 0x00) ? 0x00000000 :                                        \
    ((a) == 0xFF) ? ((0xFF << 24) | ((r) << 16) | ((g) << 8) | (b))     \
                  : ((a) << 24) |                                       \
                    (GFX_PREMULTIPLY(r,a) << 16) |                      \
                    (GFX_PREMULTIPLY(g,a) << 8) |                       \
                    (GFX_PREMULTIPLY(b,a))









struct THEBES_API gfxRGBA {
    gfxFloat r, g, b, a;

    enum PackedColorType {
        PACKED_ABGR,
        PACKED_ABGR_PREMULTIPLIED,

        PACKED_ARGB,
        PACKED_ARGB_PREMULTIPLIED,

        PACKED_XBGR,
        PACKED_XRGB
    };

    gfxRGBA() { }
    gfxRGBA(const gfxRGBA& c) : r(c.r), g(c.g), b(c.b), a(c.a) {}
    



    gfxRGBA(gfxFloat _r, gfxFloat _g, gfxFloat _b, gfxFloat _a=1.0) : r(_r), g(_g), b(_b), a(_a) {}

    






    gfxRGBA(PRUint32 c, PackedColorType colorType = PACKED_ABGR) {
        if (colorType == PACKED_ABGR ||
            colorType == PACKED_XBGR ||
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
        } else if (colorType == PACKED_XBGR ||
                   colorType == PACKED_XRGB)
        {
            a = 1.0;
        }
    }

    



#if 0
    gfxRGBA(const char* str) {
        a = 1.0;
        
        
        
    }
#endif

    







    PRUint32 Packed(PackedColorType colorType = PACKED_ABGR) const {
        gfxFloat rb = (r * 255.0);
        gfxFloat gb = (g * 255.0);
        gfxFloat bb = (b * 255.0);
        gfxFloat ab = (a * 255.0);

        if (colorType == PACKED_ABGR || colorType == PACKED_XBGR) {
            return (PRUint8(ab) << 24) |
                   (PRUint8(bb) << 16) |
                   (PRUint8(gb) << 8) |
                   (PRUint8(rb) << 0);
        } else if (colorType == PACKED_ARGB || colorType == PACKED_XRGB) {
            return (PRUint8(ab) << 24) |
                   (PRUint8(rb) << 16) |
                   (PRUint8(gb) << 8) |
                   (PRUint8(bb) << 0);
        }

        rb = (r*a) * 255.0;
        gb = (g*a) * 255.0;
        bb = (b*a) * 255.0;

        if (colorType == PACKED_ABGR_PREMULTIPLIED) {
            return (((PRUint8)(ab) << 24) |
                    ((PRUint8)(bb) << 16) |
                    ((PRUint8)(gb) << 8) |
                    ((PRUint8)(rb) << 0));
        } else if (colorType == PACKED_ARGB_PREMULTIPLIED) {
            return (((PRUint8)(ab) << 24) |
                    ((PRUint8)(rb) << 16) |
                    ((PRUint8)(gb) << 8) |
                    ((PRUint8)(bb) << 0));
        }

        return 0;
    }

#ifdef MOZILLA_INTERNAL_API
    



    
    
    void Hex(nsACString& result) const {
        nsPrintfCString hex(8, "%02x%02x%02x", PRUint8(r*255.0), PRUint8(g*255.0), PRUint8(b*255.0));
        result.Assign(hex);
    }
#endif

};

#endif 
