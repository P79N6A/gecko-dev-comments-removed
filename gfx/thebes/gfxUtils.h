




































#ifndef GFX_UTILS_H
#define GFX_UTILS_H

#include "gfxTypes.h"
#include "gfxPattern.h"
#include "gfxImageSurface.h"

class gfxDrawable;
class nsIntRegion;
struct nsIntRect;

class THEBES_API gfxUtils {
public:
    









    static void PremultiplyImageSurface(gfxImageSurface *aSourceSurface,
                                        gfxImageSurface *aDestSurface = nsnull);
    static void UnpremultiplyImageSurface(gfxImageSurface *aSurface,
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

    




    static PRBool GfxRectToIntRect(const gfxRect& aIn, nsIntRect* aOut);

    



    static gfxFloat ClampToScaleFactor(gfxFloat aVal);
};

#endif
