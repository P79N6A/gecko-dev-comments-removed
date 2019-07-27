




#ifndef GFX_COLOR_H
#define GFX_COLOR_H

#include "gfxTypes.h"

#include "mozilla/Attributes.h" 
#include "mozilla/Endian.h" 

#define GFX_UINT32_FROM_BPTR(pbptr,i) (((uint32_t*)(pbptr))[i])











#define GFX_0XFF_PPIXEL_FROM_UINT32(x) \
  ( (mozilla::NativeEndian::swapToBigEndian(uint32_t(x)) >> 8) | (0xFFU << 24) )

#define GFX_0XFF_PPIXEL_FROM_BPTR(x) \
     ( GFX_0XFF_PPIXEL_FROM_UINT32(GFX_UINT32_FROM_BPTR((x),0)) )









#define GFX_BLOCK_RGB_TO_FRGB(from,to) \
  PR_BEGIN_MACRO \
    uint32_t m0 = GFX_UINT32_FROM_BPTR(from,0), \
             m1 = GFX_UINT32_FROM_BPTR(from,1), \
             m2 = GFX_UINT32_FROM_BPTR(from,2), \
             rgbr = mozilla::NativeEndian::swapToBigEndian(m0), \
             gbrg = mozilla::NativeEndian::swapToBigEndian(m1), \
             brgb = mozilla::NativeEndian::swapToBigEndian(m2), \
             p0, p1, p2, p3; \
    p0 = 0xFF000000 | ((rgbr) >>  8); \
    p1 = 0xFF000000 | ((rgbr) << 16) | ((gbrg) >> 16); \
    p2 = 0xFF000000 | ((gbrg) <<  8) | ((brgb) >> 24); \
    p3 = 0xFF000000 | (brgb); \
    to[0] = p0; to[1] = p1; to[2] = p2; to[3] = p3; \
  PR_END_MACRO









#define GFX_DIVIDE_BY_255(v)  \
     (((((unsigned)(v)) << 8) + ((unsigned)(v)) + 255) >> 16)






uint8_t MOZ_ALWAYS_INLINE gfxPreMultiply(uint8_t c, uint8_t a) {
    return GFX_DIVIDE_BY_255((c)*(a));
}





uint32_t MOZ_ALWAYS_INLINE
gfxPackedPixelNoPreMultiply(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    return (((a) << 24) | ((r) << 16) | ((g) << 8) | (b));
}





uint32_t MOZ_ALWAYS_INLINE
gfxPackedPixel(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
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








struct gfxRGBA {
    gfxFloat r, g, b, a;

    enum PackedColorType {
        PACKED_ABGR,
        PACKED_ABGR_PREMULTIPLIED,

        PACKED_ARGB,
        PACKED_ARGB_PREMULTIPLIED,

        PACKED_XRGB
    };

    gfxRGBA() { }
    



    MOZ_CONSTEXPR gfxRGBA(gfxFloat _r, gfxFloat _g, gfxFloat _b, gfxFloat _a=1.0) : r(_r), g(_g), b(_b), a(_a) {}

    








    MOZ_IMPLICIT gfxRGBA(uint32_t c, PackedColorType colorType = PACKED_ABGR) {
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
