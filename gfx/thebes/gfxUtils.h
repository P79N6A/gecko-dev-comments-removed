




#ifndef GFX_UTILS_H
#define GFX_UTILS_H

#include "gfxTypes.h"
#include "gfxPattern.h"
#include "gfxImageSurface.h"
#include "ImageContainer.h"
#include "mozilla/gfx/2D.h"
#include "imgIContainer.h"

class gfxDrawable;
class nsIntRegion;
struct nsIntRect;

class THEBES_API gfxUtils {
public:
    









    static void PremultiplyImageSurface(gfxImageSurface *aSourceSurface,
                                        gfxImageSurface *aDestSurface = nullptr);
    static void UnpremultiplyImageSurface(gfxImageSurface *aSurface,
                                          gfxImageSurface *aDestSurface = nullptr);

    static void ConvertBGRAtoRGBA(gfxImageSurface *aSourceSurface,
                                  gfxImageSurface *aDestSurface = nullptr);

    












    static void DrawPixelSnapped(gfxContext*      aContext,
                                 gfxDrawable*     aDrawable,
                                 const gfxMatrix& aUserSpaceToImageSpace,
                                 const gfxRect&   aSubimage,
                                 const gfxRect&   aSourceRect,
                                 const gfxRect&   aImageRect,
                                 const gfxRect&   aFill,
                                 const gfxImageSurface::gfxImageFormat aFormat,
                                 gfxPattern::GraphicsFilter aFilter,
                                 PRUint32         aImageFlags = imgIContainer::FLAG_NONE);

    


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












static inline bool
IsPowerOfTwo(int aNumber)
{
    return (aNumber & (aNumber - 1)) == 0;
}





static inline int
NextPowerOfTwo(int aNumber)
{
#if defined(__arm__)
    return 1 << (32 - __builtin_clz(aNumber - 1));
#else
    --aNumber;
    aNumber |= aNumber >> 1;
    aNumber |= aNumber >> 2;
    aNumber |= aNumber >> 4;
    aNumber |= aNumber >> 8;
    aNumber |= aNumber >> 16;
    return ++aNumber;
#endif
}

} 
} 

#endif
