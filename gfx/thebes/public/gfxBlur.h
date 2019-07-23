




































#ifndef GFX_BLUR_H
#define GFX_BLUR_H

#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "gfxRect.h"
#include "gfxTypes.h"












class THEBES_API gfxAlphaBoxBlur
{
public:
    gfxAlphaBoxBlur();

    ~gfxAlphaBoxBlur();

    








    gfxContext* Init(const gfxRect& aRect,
                     const gfxIntSize& aBlurRadius,
                     const gfxRect* aDirtyRect);

    




    gfxContext* GetContext()
    {
        return mContext;
    }

    


    void PremultiplyAlpha(gfxFloat alpha);

    







    void Paint(gfxContext* aDestinationCtx, const gfxPoint& offset = gfxPoint(0.0, 0.0));

    



    static gfxIntSize CalculateBlurRadius(const gfxPoint& aStandardDeviation);

protected:
    


    gfxIntSize mBlurRadius;

    


    nsRefPtr<gfxContext> mContext;

    


    nsRefPtr<gfxImageSurface> mImageSurface;

    



    gfxRect mDirtyRect;
    PRBool mHasDirtyRect;
};

#endif 
