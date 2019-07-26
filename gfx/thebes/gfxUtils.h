




#ifndef GFX_UTILS_H
#define GFX_UTILS_H

#include "gfxTypes.h"
#include "GraphicsFilter.h"
#include "imgIContainer.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/RefPtr.h"

class gfxDrawable;
class nsIntRegion;
struct nsIntRect;

namespace mozilla {
namespace layers {
class PlanarYCbCrData;
}
}

class gfxUtils {
public:
    typedef mozilla::gfx::DataSourceSurface DataSourceSurface;
    typedef mozilla::gfx::IntPoint IntPoint;
    typedef mozilla::gfx::Matrix Matrix;
    typedef mozilla::gfx::SourceSurface SourceSurface;
    typedef mozilla::gfx::SurfaceFormat SurfaceFormat;

    









    static bool PremultiplyDataSurface(DataSourceSurface* srcSurf,
                                       DataSourceSurface* destSurf);
    static bool UnpremultiplyDataSurface(DataSourceSurface* srcSurf,
                                         DataSourceSurface* destSurf);

    static mozilla::TemporaryRef<DataSourceSurface>
      CreatePremultipliedDataSurface(DataSourceSurface* srcSurf);
    static mozilla::TemporaryRef<DataSourceSurface>
      CreateUnpremultipliedDataSurface(DataSourceSurface* srcSurf);

    static void ConvertBGRAtoRGBA(uint8_t* aData, uint32_t aLength);

    












    static void DrawPixelSnapped(gfxContext*      aContext,
                                 gfxDrawable*     aDrawable,
                                 const gfxMatrix& aUserSpaceToImageSpace,
                                 const gfxRect&   aSubimage,
                                 const gfxRect&   aSourceRect,
                                 const gfxRect&   aImageRect,
                                 const gfxRect&   aFill,
                                 const gfxImageFormat aFormat,
                                 GraphicsFilter aFilter,
                                 uint32_t         aImageFlags = imgIContainer::FLAG_NONE);

    


    static void ClipToRegion(gfxContext* aContext, const nsIntRegion& aRegion);

    


    static void ClipToRegion(mozilla::gfx::DrawTarget* aTarget, const nsIntRegion& aRegion);

    


    static void ClipToRegionSnapped(gfxContext* aContext, const nsIntRegion& aRegion);

    


    static void ClipToRegionSnapped(mozilla::gfx::DrawTarget* aTarget, const nsIntRegion& aRegion);

    


    static void PathFromRegion(gfxContext* aContext, const nsIntRegion& aRegion);

    


    static void PathFromRegionSnapped(gfxContext* aContext, const nsIntRegion& aRegion);

    


    static int ImageFormatToDepth(gfxImageFormat aFormat);

    




    static gfxMatrix TransformRectToRect(const gfxRect& aFrom,
                                         const gfxPoint& aToTopLeft,
                                         const gfxPoint& aToTopRight,
                                         const gfxPoint& aToBottomRight);

    static Matrix TransformRectToRect(const gfxRect& aFrom,
                                      const IntPoint& aToTopLeft,
                                      const IntPoint& aToTopRight,
                                      const IntPoint& aToBottomRight);

    




    static bool GfxRectToIntRect(const gfxRect& aIn, nsIntRect* aOut);

    



    static gfxFloat ClampToScaleFactor(gfxFloat aVal);

    








    static void
    GetYCbCrToRGBDestFormatAndSize(const mozilla::layers::PlanarYCbCrData& aData,
                                   gfxImageFormat& aSuggestedFormat,
                                   gfxIntSize& aSuggestedSize);

    




    static void
    ConvertYCbCrToRGB(const mozilla::layers::PlanarYCbCrData& aData,
                      const gfxImageFormat& aDestFormat,
                      const gfxIntSize& aDestSize,
                      unsigned char* aDestBuffer,
                      int32_t aStride);

    








































    static mozilla::TemporaryRef<DataSourceSurface>
    CopySurfaceToDataSourceSurfaceWithFormat(SourceSurface* aSurface,
                                             SurfaceFormat aFormat);

    static const uint8_t sUnpremultiplyTable[256*256];
    static const uint8_t sPremultiplyTable[256*256];

    




    static const mozilla::gfx::Color& GetColorForFrameNumber(uint64_t aFrameNumber);
    static const uint32_t sNumFrameColors;

#ifdef MOZ_DUMP_PAINTING
    


    static void WriteAsPNG(mozilla::gfx::DrawTarget* aDT, const char* aFile);

    


    static void DumpAsDataURL(mozilla::gfx::DrawTarget* aDT);

    


    static void CopyAsDataURL(mozilla::gfx::DrawTarget* aDT);

    static bool DumpPaintList();

    static bool sDumpPainting;
    static bool sDumpPaintingToFile;
    static FILE* sDumpPaintFile;

    



    static void WriteAsPNG(mozilla::RefPtr<mozilla::gfx::SourceSurface> aSourceSurface, const char* aFile);

    



    static void DumpAsDataURL(mozilla::RefPtr<mozilla::gfx::SourceSurface> aSourceSurface);

    



    static void CopyAsDataURL(mozilla::RefPtr<mozilla::gfx::SourceSurface> aSourceSurface);
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
