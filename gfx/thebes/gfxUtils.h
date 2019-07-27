




#ifndef GFX_UTILS_H
#define GFX_UTILS_H

#include "gfxColor.h"
#include "gfxTypes.h"
#include "GraphicsFilter.h"
#include "imgIContainer.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/RefPtr.h"
#include "nsPrintfCString.h"

class gfxASurface;
class gfxDrawable;
class nsIntRegion;
class nsIPresShell;

struct nsIntRect;

namespace mozilla {
namespace layers {
struct PlanarYCbCrData;
}
namespace image {
class ImageRegion;
}
}

class gfxUtils {
public:
    typedef mozilla::gfx::DataSourceSurface DataSourceSurface;
    typedef mozilla::gfx::DrawTarget DrawTarget;
    typedef mozilla::gfx::IntPoint IntPoint;
    typedef mozilla::gfx::Matrix Matrix;
    typedef mozilla::gfx::SourceSurface SourceSurface;
    typedef mozilla::gfx::SurfaceFormat SurfaceFormat;
    typedef mozilla::image::ImageRegion ImageRegion;

    









    static bool PremultiplyDataSurface(DataSourceSurface* srcSurf,
                                       DataSourceSurface* destSurf);
    static bool UnpremultiplyDataSurface(DataSourceSurface* srcSurf,
                                         DataSourceSurface* destSurf);

    static mozilla::TemporaryRef<DataSourceSurface>
      CreatePremultipliedDataSurface(DataSourceSurface* srcSurf);
    static mozilla::TemporaryRef<DataSourceSurface>
      CreateUnpremultipliedDataSurface(DataSourceSurface* srcSurf);

    static void ConvertBGRAtoRGBA(uint8_t* aData, uint32_t aLength);

    












    static void DrawPixelSnapped(gfxContext*        aContext,
                                 gfxDrawable*       aDrawable,
                                 const gfxSize&     aImageSize,
                                 const ImageRegion& aRegion,
                                 const mozilla::gfx::SurfaceFormat aFormat,
                                 GraphicsFilter     aFilter,
                                 uint32_t           aImageFlags = imgIContainer::FLAG_NONE,
                                 gfxFloat           aOpacity = 1.0);

    


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

    


    static void ClearThebesSurface(gfxASurface* aSurface,
                                   nsIntRect* aRect = nullptr,
                                   const gfxRGBA& aColor = gfxRGBA(0.0, 0.0, 0.0, 0.0));

    








































    static mozilla::TemporaryRef<DataSourceSurface>
    CopySurfaceToDataSourceSurfaceWithFormat(SourceSurface* aSurface,
                                             SurfaceFormat aFormat);

    static const uint8_t sUnpremultiplyTable[256*256];
    static const uint8_t sPremultiplyTable[256*256];

    




    static const mozilla::gfx::Color& GetColorForFrameNumber(uint64_t aFrameNumber);
    static const uint32_t sNumFrameColors;


    enum BinaryOrData {
        eBinaryEncode,
        eDataURIEncode
    };

    





















    static nsresult
    EncodeSourceSurface(SourceSurface* aSurface,
                        const nsACString& aMimeType,
                        const nsAString& aOutputOptions,
                        BinaryOrData aBinaryOrData,
                        FILE* aFile);

    


    static void WriteAsPNG(SourceSurface* aSurface, const nsAString& aFile);
    static void WriteAsPNG(SourceSurface* aSurface, const char* aFile);
    static void WriteAsPNG(DrawTarget* aDT, const nsAString& aFile);
    static void WriteAsPNG(DrawTarget* aDT, const char* aFile);
    static void WriteAsPNG(nsIPresShell* aShell, const char* aFile);

    






    static void DumpAsDataURI(SourceSurface* aSourceSurface, FILE* aFile);
    static inline void DumpAsDataURI(SourceSurface* aSourceSurface) {
        DumpAsDataURI(aSourceSurface, stdout);
    }
    static void DumpAsDataURI(DrawTarget* aDT, FILE* aFile);
    static inline void DumpAsDataURI(DrawTarget* aDT) {
        DumpAsDataURI(aDT, stdout);
    }

    


    static void CopyAsDataURI(SourceSurface* aSourceSurface);
    static void CopyAsDataURI(DrawTarget* aDT);

#ifdef MOZ_DUMP_PAINTING
    static bool DumpPaintList();

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
