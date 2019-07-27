




#ifndef GFX_BLUR_H
#define GFX_BLUR_H

#include "gfxTypes.h"
#include "nsSize.h"
#include "nsAutoPtr.h"
#include "gfxPoint.h"
#include "mozilla/RefPtr.h"
#include "mozilla/UniquePtr.h"

class gfxContext;
struct gfxRect;
struct gfxRGBA;
struct gfxCornerSizes;
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

    mozilla::TemporaryRef<mozilla::gfx::SourceSurface> DoBlur(mozilla::gfx::DrawTarget* aDT, mozilla::gfx::IntPoint* aTopLeft);

    







    void Paint(gfxContext* aDestinationCtx);

    





    static gfxIntSize CalculateBlurRadius(const gfxPoint& aStandardDeviation);

    
















    static void BlurRectangle(gfxContext *aDestinationCtx,
                              const gfxRect& aRect,
                              gfxCornerSizes* aCornerRadii,
                              const gfxPoint& aBlurStdDev,
                              const gfxRGBA& aShadowColor,
                              const gfxRect& aDirtyRect,
                              const gfxRect& aSkipRect);

    static void ShutdownBlurCache();



protected:
    


    nsRefPtr<gfxContext> mContext;

    


    nsAutoArrayPtr<unsigned char> mData;

     


    mozilla::UniquePtr<mozilla::gfx::AlphaBoxBlur> mBlur;
};

#endif 
