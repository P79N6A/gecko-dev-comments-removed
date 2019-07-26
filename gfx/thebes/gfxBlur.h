




#ifndef GFX_BLUR_H
#define GFX_BLUR_H

#include "gfxTypes.h"
#include "nsSize.h"
#include "nsAutoPtr.h"
#include "gfxPoint.h"

class gfxContext;
class gfxImageSurface;
struct gfxRect;
struct gfxRGBA;
class gfxCornerSizes;
class gfxMatrix;

namespace mozilla {
  namespace gfx {
    class AlphaBoxBlur;
  }
}




















class gfxAlphaBoxBlur
{
public:
    gfxAlphaBoxBlur();

    ~gfxAlphaBoxBlur();

    

















    gfxContext* Init(const gfxRect& aRect,
                     const gfxIntSize& aSpreadRadius,
                     const gfxIntSize& aBlurRadius,
                     const gfxRect* aDirtyRect,
                     const gfxRect* aSkipRect);

    




    gfxContext* GetContext()
    {
        return mContext;
    }

    







    void Paint(gfxContext* aDestinationCtx);

    





    static gfxIntSize CalculateBlurRadius(const gfxPoint& aStandardDeviation);

    static void BlurRectangle(gfxContext *aDestinationCtx,
                              const gfxRect& aRect,
                              gfxCornerSizes* aCornerRadii,
                              const gfxIntSize& aBlurRadius,
                              const gfxRGBA& aShadowColor,
                              const gfxRect& aDirtyRect,
                              const gfxRect& aSkipRect);



protected:
    


    nsRefPtr<gfxContext> mContext;

    


    nsRefPtr<gfxImageSurface> mImageSurface;

     


    mozilla::gfx::AlphaBoxBlur *mBlur;
};

#endif 
