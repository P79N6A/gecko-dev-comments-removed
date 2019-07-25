




































#ifndef GFX_UTILS_H
#define GFX_UTILS_H

#include "gfxTypes.h"
#include "gfxPattern.h"
#include "gfxImageSurface.h"
#include "ImageLayers.h"
#include "mozilla/gfx/2D.h"

class gfxDrawable;
class nsIntRegion;
struct nsIntRect;

class THEBES_API gfxUtils {
public:
    









    static void PremultiplyImageSurface(gfxImageSurface *aSourceSurface,
                                        gfxImageSurface *aDestSurface = nsnull);
    static void UnpremultiplyImageSurface(gfxImageSurface *aSurface,
                                          gfxImageSurface *aDestSurface = nsnull);

    static void ConvertBGRAtoRGBA(gfxImageSurface *aSourceSurface,
                                  gfxImageSurface *aDestSurface = nsnull);

    












    static void DrawPixelSnapped(gfxContext*      aContext,
                                 gfxDrawable*     aDrawable,
                                 const gfxMatrix& aUserSpaceToImageSpace,
                                 const gfxRect&   aSubimage,
                                 const gfxRect&   aSourceRect,
                                 const gfxRect&   aImageRect,
                                 const gfxRect&   aFill,
                                 const gfxImageSurface::gfxImageFormat aFormat,
                                 const gfxPattern::GraphicsFilter& aFilter);

    


    static void ClipToRegion(gfxContext* aContext, const nsIntRegion& aRegion);

    


    static void ClipToRegionSnapped(gfxContext* aContext, const nsIntRegion& aRegion);

    


    static void PathFromRegion(gfxContext* aContext, const nsIntRegion& aRegion);

    


    static void PathFromRegionSnapped(gfxContext* aContext, const nsIntRegion& aRegion);

    


    static int ImageFormatToDepth(gfxASurface::gfxImageFormat aFormat);

    




    static bool GfxRectToIntRect(const gfxRect& aIn, nsIntRect* aOut);

    



    static gfxFloat ClampToScaleFactor(gfxFloat aVal);

    








    static void
    GetYCbCrToRGBDestFormatAndSize(const mozilla::layers::PlanarYCbCrImage::Data& aData,
                                   gfxASurface::gfxImageFormat& aSuggestedFormat,
                                   gfxIntSize& aSuggestedSize);

    




    static void
    ConvertYCbCrToRGB(const mozilla::layers::PlanarYCbCrImage::Data& aData,
                      const gfxASurface::gfxImageFormat& aDestFormat,
                      const gfxIntSize& aDestSize,
                      unsigned char* aDestBuffer,
                      PRInt32 aStride);

#ifdef MOZ_DUMP_PAINTING
    


    static void WriteAsPNG(mozilla::gfx::DrawTarget* aDT, const char* aFile);

    


    static void DumpAsDataURL(mozilla::gfx::DrawTarget* aDT);

    


    static void CopyAsDataURL(mozilla::gfx::DrawTarget* aDT);

    static bool sDumpPaintList;
    static bool sDumpPainting;
    static bool sDumpPaintingToFile;
    static FILE* sDumpPaintFile;
#endif
};

namespace mozilla {
namespace gfx {










#if defined(__arm__)
    #define CountLeadingZeroes(x) __builtin_clz(x)
#else

#define sub_shift(zeros, x, n)  \
    zeros -= n;                 \
    x >>= n

static inline int CountLeadingZeroes(uint32_t aNumber)
{
    if (aNumber == 0) {
        return 32;
    }
    int zeros = 31;
    if (aNumber & 0xFFFF0000) {
        sub_shift(zeros, aNumber, 16);
    }
    if (aNumber & 0xFF00) {
        sub_shift(zeros, aNumber, 8);
    }
    if (aNumber & 0xF0) {
        sub_shift(zeros, aNumber, 4);
    }
    if (aNumber & 0xC) {
        sub_shift(zeros, aNumber, 2);
    }
    if (aNumber & 0x2) {
        sub_shift(zeros, aNumber, 1);
    }
    return zeros;
}
#endif




static inline bool
IsPowerOfTwo(int aNumber)
{
    return (aNumber & (aNumber - 1)) == 0;
}





static inline int
NextPowerOfTwo(int aNumber)
{
    return 1 << (32 - CountLeadingZeroes(aNumber - 1));
}

} 
} 

#endif
