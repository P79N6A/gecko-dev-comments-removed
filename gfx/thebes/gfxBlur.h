




#ifndef GFX_BLUR_H
#define GFX_BLUR_H

#include "gfxTypes.h"
#include "nsSize.h"
#include "nsAutoPtr.h"
#include "gfxPoint.h"
#include "mozilla/RefPtr.h"

class gfxContext;
struct gfxRect;
struct gfxRGBA;
class gfxCornerSizes;
class gfxMatrix;

namespace mozilla {
  namespace gfx {
    class AlphaBoxBlur;
    class SourceSurface;
    class DrawTarget;
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
                              const gfxPoint& aBlurStdDev,
                              const gfxRGBA& aShadowColor,
                              const gfxRect& aDirtyRect,
                              const gfxRect& aSkipRect);



protected:
    


    nsRefPtr<gfxContext> mContext;

    


    nsAutoArrayPtr<unsigned char> mData;

     


    mozilla::gfx::AlphaBoxBlur *mBlur;
};

#endif 
