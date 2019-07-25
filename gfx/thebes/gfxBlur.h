




































#ifndef GFX_BLUR_H
#define GFX_BLUR_H

#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "gfxTypes.h"
#include "gfxUtils.h"
#include "nsRect.h"




















class THEBES_API gfxAlphaBoxBlur
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

    







    void Paint(gfxContext* aDestinationCtx, const gfxPoint& offset = gfxPoint(0.0, 0.0));

    





    static gfxIntSize CalculateBlurRadius(const gfxPoint& aStandardDeviation);

protected:
    


    gfxIntSize mSpreadRadius;
    


    gfxIntSize mBlurRadius;

    


    nsRefPtr<gfxContext> mContext;

    


    nsRefPtr<gfxImageSurface> mImageSurface;

    



    gfxRect mDirtyRect;
    



    nsIntRect mSkipRect;

    bool mHasDirtyRect;
};

#endif 
