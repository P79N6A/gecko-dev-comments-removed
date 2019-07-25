



































#include "gfxDrawable.h"
#include "gfxASurface.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#ifdef MOZ_X11
#include "cairo.h"
#include "gfxXlibSurface.h"
#endif

gfxSurfaceDrawable::gfxSurfaceDrawable(gfxASurface* aSurface,
                                       const gfxIntSize aSize,
                                       const gfxMatrix aTransform)
 : gfxDrawable(aSize)
 , mSurface(aSurface)
 , mTransform(aTransform)
{
}

static gfxMatrix
DeviceToImageTransform(gfxContext* aContext,
                       const gfxMatrix& aUserSpaceToImageSpace)
{
    gfxFloat deviceX, deviceY;
    nsRefPtr<gfxASurface> currentTarget =
        aContext->CurrentSurface(&deviceX, &deviceY);
    gfxMatrix currentMatrix = aContext->CurrentMatrix();
    gfxMatrix deviceToUser = gfxMatrix(currentMatrix).Invert();
    deviceToUser.Translate(-gfxPoint(-deviceX, -deviceY));
    return gfxMatrix(deviceToUser).Multiply(aUserSpaceToImageSpace);
}

static void
PreparePatternForUntiledDrawing(gfxPattern* aPattern,
                                const gfxMatrix& aDeviceToImage,
                                gfxASurface::gfxSurfaceType aSurfaceType,
                                nsRefPtr<gfxASurface> currentTarget,
                                const gfxPattern::GraphicsFilter aDefaultFilter)
{
    
    
    
    switch (aSurfaceType) {
        case gfxASurface::SurfaceTypeXlib:
        case gfxASurface::SurfaceTypeXcb:
        {
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            

            PRBool isDownscale =
                aDeviceToImage.xx >= 1.0 && aDeviceToImage.yy >= 1.0 &&
                aDeviceToImage.xy == 0.0 && aDeviceToImage.yx == 0.0;
            if (!isDownscale) {
#ifdef MOZ_X11
                PRBool fastExtendPad = PR_FALSE;
                if (currentTarget->GetType() == gfxASurface::SurfaceTypeXlib &&
                    cairo_version() >= CAIRO_VERSION_ENCODE(1,9,2)) {
                    gfxXlibSurface *xlibSurface =
                        static_cast<gfxXlibSurface *>(currentTarget.get());
                    Display *dpy = xlibSurface->XDisplay();
                    
                    
                    if (VendorRelease (dpy) < 60700000 &&
                        VendorRelease (dpy) >= 10699000)
                        fastExtendPad = PR_TRUE;
                }
                if (fastExtendPad) {
                    aPattern->SetExtend(gfxPattern::EXTEND_PAD);
                    aPattern->SetFilter(aDefaultFilter);
                } else
#endif
                    aPattern->SetFilter(gfxPattern::FILTER_FAST);
            }
            break;
        }

        case gfxASurface::SurfaceTypeQuartz:
        case gfxASurface::SurfaceTypeQuartzImage:
            
            aPattern->SetFilter(aDefaultFilter);
            break;

        default:
            
            
            
            aPattern->SetExtend(gfxPattern::EXTEND_PAD);
            aPattern->SetFilter(aDefaultFilter);
            break;
    }
}

PRBool
gfxSurfaceDrawable::Draw(gfxContext* aContext,
                         const gfxRect& aFillRect,
                         PRBool aRepeat,
                         const gfxPattern::GraphicsFilter& aFilter,
                         const gfxMatrix& aTransform)
{
    nsRefPtr<gfxPattern> pattern = new gfxPattern(mSurface);
    if (aRepeat) {
        pattern->SetExtend(gfxPattern::EXTEND_REPEAT);
        pattern->SetFilter(aFilter);
    } else {
        gfxPattern::GraphicsFilter filter = aFilter;
        if (aContext->CurrentMatrix().HasOnlyIntegerTranslation() &&
            aTransform.HasOnlyIntegerTranslation())
        {
          
          
          
          filter = gfxPattern::FILTER_FAST;
        }
        nsRefPtr<gfxASurface> currentTarget = aContext->CurrentSurface();
        gfxASurface::gfxSurfaceType surfaceType = currentTarget->GetType();
        gfxMatrix deviceSpaceToImageSpace =
            DeviceToImageTransform(aContext, aTransform);
        PreparePatternForUntiledDrawing(pattern, deviceSpaceToImageSpace,
                                        surfaceType, currentTarget, filter);
    }
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
    pattern->SetFilter(gfxPattern::FILTER_FAST); 
#endif
    pattern->SetMatrix(gfxMatrix(aTransform).Multiply(mTransform));
    aContext->NewPath();
    aContext->SetPattern(pattern);
    aContext->Rectangle(aFillRect);
    aContext->Fill();
    return PR_TRUE;
}

gfxCallbackDrawable::gfxCallbackDrawable(gfxDrawingCallback* aCallback,
                                         const gfxIntSize aSize)
 : gfxDrawable(aSize)
 , mCallback(aCallback)
{
}

already_AddRefed<gfxSurfaceDrawable>
gfxCallbackDrawable::MakeSurfaceDrawable(const gfxPattern::GraphicsFilter aFilter)
{
    nsRefPtr<gfxASurface> surface =
        gfxPlatform::GetPlatform()->CreateOffscreenSurface(mSize, gfxASurface::CONTENT_COLOR_ALPHA);
    if (!surface || surface->CairoStatus() != 0)
        return nsnull;

    nsRefPtr<gfxContext> ctx = new gfxContext(surface);
    Draw(ctx, gfxRect(0, 0, mSize.width, mSize.height), PR_FALSE, aFilter);
    nsRefPtr<gfxSurfaceDrawable> drawable = new gfxSurfaceDrawable(surface, mSize);
    return drawable.forget();
}

PRBool
gfxCallbackDrawable::Draw(gfxContext* aContext,
                          const gfxRect& aFillRect,
                          PRBool aRepeat,
                          const gfxPattern::GraphicsFilter& aFilter,
                          const gfxMatrix& aTransform)
{
    if (aRepeat && !mSurfaceDrawable) {
        mSurfaceDrawable = MakeSurfaceDrawable(aFilter);
    }

    if (mSurfaceDrawable)
        return mSurfaceDrawable->Draw(aContext, aFillRect, aRepeat, aFilter,
                                      aTransform);

    if (mCallback)
        return (*mCallback)(aContext, aFillRect, aFilter, aTransform);

    return PR_FALSE;
}

gfxPatternDrawable::gfxPatternDrawable(gfxPattern* aPattern,
                                       const gfxIntSize aSize)
 : gfxDrawable(aSize)
 , mPattern(aPattern)
{
}

class DrawingCallbackFromDrawable : public gfxDrawingCallback {
public:
    DrawingCallbackFromDrawable(gfxDrawable* aDrawable)
     : mDrawable(aDrawable) {
        NS_ASSERTION(aDrawable, "aDrawable is null!");
    }

    virtual ~DrawingCallbackFromDrawable() {}

    virtual PRBool operator()(gfxContext* aContext,
                              const gfxRect& aFillRect,
                              const gfxPattern::GraphicsFilter& aFilter,
                              const gfxMatrix& aTransform = gfxMatrix())
    {
        return mDrawable->Draw(aContext, aFillRect, PR_FALSE, aFilter,
                               aTransform);
    }
private:
    nsRefPtr<gfxDrawable> mDrawable;
};

already_AddRefed<gfxCallbackDrawable>
gfxPatternDrawable::MakeCallbackDrawable()
{
    nsRefPtr<gfxDrawingCallback> callback =
        new DrawingCallbackFromDrawable(this);
    nsRefPtr<gfxCallbackDrawable> callbackDrawable =
        new gfxCallbackDrawable(callback, mSize);
    return callbackDrawable.forget();
}

PRBool
gfxPatternDrawable::Draw(gfxContext* aContext,
                         const gfxRect& aFillRect,
                         PRBool aRepeat,
                         const gfxPattern::GraphicsFilter& aFilter,
                         const gfxMatrix& aTransform)
{
    if (!mPattern)
        return PR_FALSE;

    if (aRepeat) {
        
        
        
        
        
        
        
        nsRefPtr<gfxCallbackDrawable> callbackDrawable = MakeCallbackDrawable();
        return callbackDrawable->Draw(aContext, aFillRect, PR_TRUE, aFilter,
                                      aTransform);
    }

    aContext->NewPath();
    gfxMatrix oldMatrix = mPattern->GetMatrix();
    mPattern->SetMatrix(gfxMatrix(aTransform).Multiply(oldMatrix));
    aContext->SetPattern(mPattern);
    aContext->Rectangle(aFillRect);
    aContext->Fill();
    mPattern->SetMatrix(oldMatrix);
    return PR_TRUE;
}
