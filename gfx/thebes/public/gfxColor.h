




































#ifndef GFX_COLOR_H
#define GFX_COLOR_H

#include "nsPrintfCString.h"

#include "gfxTypes.h"









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
            r = ((c >> 0) & 0xff) / 255.0;
            g = ((c >> 8) & 0xff) / 255.0;
            b = ((c >> 16) & 0xff) / 255.0;
            a = ((c >> 24) & 0xff) / 255.0;
        } else if (colorType == PACKED_ARGB ||
                   colorType == PACKED_XRGB ||
                   colorType == PACKED_ARGB_PREMULTIPLIED)
        {
            b = ((c >> 0) & 0xff) / 255.0;
            g = ((c >> 8) & 0xff) / 255.0;
            r = ((c >> 16) & 0xff) / 255.0;
            a = ((c >> 24) & 0xff) / 255.0;
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
        if (colorType == PACKED_ABGR || colorType == PACKED_XBGR) {
            return (((PRUint8)(a * 255.0) << 24) |
                    ((PRUint8)(b * 255.0) << 16) |
                    ((PRUint8)(g * 255.0) << 8) |
                    ((PRUint8)(r * 255.0)));
        } else if (colorType == PACKED_ARGB || colorType == PACKED_XRGB) {
            return (((PRUint8)(a * 255.0) << 24) |
                    ((PRUint8)(r * 255.0) << 16) |
                    ((PRUint8)(g * 255.0) << 8) |
                    ((PRUint8)(b * 255.0)));
        } else if (colorType == PACKED_ABGR_PREMULTIPLIED) {
            return (((PRUint8)(a * 255.0) << 24) |
                    ((PRUint8)((b*a) * 255.0) << 16) |
                    ((PRUint8)((g*a) * 255.0) << 8) |
                    ((PRUint8)((r*a) * 255.0)));
        } else if (colorType == PACKED_ARGB_PREMULTIPLIED) {
            return (((PRUint8)(a * 255.0) << 24) |
                    ((PRUint8)((r*a) * 255.0) << 16) |
                    ((PRUint8)((g*a) * 255.0) << 8) |
                    ((PRUint8)((b*a) * 255.0)));
        } else {
            return 0;
        }
    }

    



    
    
    void Hex(nsACString& result) const {
        nsPrintfCString hex(8, "%02x%02x%02x", PRUint8(r*255.0), PRUint8(g*255.0), PRUint8(b*255.0));
        result.Assign(hex);
    }

};

#endif 
