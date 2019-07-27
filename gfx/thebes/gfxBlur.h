




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

namespace mozilla {
  namespace gfx {
    class AlphaBoxBlur;
    struct RectCornerRadii;
    class SourceSurface;
    class DrawTarget;
  }
}




















class gfxAlphaBoxBlur
{
    typedef mozilla::gfx::RectCornerRadii RectCornerRadii;

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
                              RectCornerRadii* aCornerRadii,
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
