



































#include "gfxDrawable.h"
#include "gfxASurface.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "mozilla/arm.h"
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
                                gfxASurface *currentTarget,
                                const gfxPattern::GraphicsFilter aDefaultFilter)
{
    
    
    
    switch (currentTarget->GetType()) {

        
        
        
        
        
        
        
        
        case gfxASurface::SurfaceTypePDF:
        case gfxASurface::SurfaceTypePS:
        case gfxASurface::SurfaceTypeWin32Printing:
            aPattern->SetExtend(gfxPattern::EXTEND_NONE);
            aPattern->SetFilter(aDefaultFilter);
            break;

#ifdef MOZ_X11
        case gfxASurface::SurfaceTypeXlib:
        {
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            

            gfxXlibSurface *xlibSurface =
                static_cast<gfxXlibSurface *>(currentTarget);
            Display *dpy = xlibSurface->XDisplay();
            
            
            if (VendorRelease(dpy) >= 60700000 ||
                VendorRelease(dpy) < 10699000) {

                bool isDownscale =
                    aDeviceToImage.xx >= 1.0 && aDeviceToImage.yy >= 1.0 &&
                    aDeviceToImage.xy == 0.0 && aDeviceToImage.yx == 0.0;

                gfxPattern::GraphicsFilter filter =
                    isDownscale ? aDefaultFilter : gfxPattern::FILTER_FAST;
                aPattern->SetFilter(filter);

                
                break;
            }
            
        }
#endif

        default:
            
            
            
            aPattern->SetExtend(gfxPattern::EXTEND_PAD);
            aPattern->SetFilter(aDefaultFilter);
            break;
    }
}

bool
gfxSurfaceDrawable::Draw(gfxContext* aContext,
                         const gfxRect& aFillRect,
                         bool aRepeat,
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
        gfxMatrix deviceSpaceToImageSpace =
            DeviceToImageTransform(aContext, aTransform);
        PreparePatternForUntiledDrawing(pattern, deviceSpaceToImageSpace,
                                        currentTarget, filter);
    }
    pattern->SetMatrix(gfxMatrix(aTransform).Multiply(mTransform));
    aContext->NewPath();
    aContext->SetPattern(pattern);
    aContext->Rectangle(aFillRect);
    aContext->Fill();
    return true;
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
    Draw(ctx, gfxRect(0, 0, mSize.width, mSize.height), false, aFilter);
    nsRefPtr<gfxSurfaceDrawable> drawable = new gfxSurfaceDrawable(surface, mSize);
    return drawable.forget();
}

bool
gfxCallbackDrawable::Draw(gfxContext* aContext,
                          const gfxRect& aFillRect,
                          bool aRepeat,
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

    return false;
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

    virtual bool operator()(gfxContext* aContext,
                              const gfxRect& aFillRect,
                              const gfxPattern::GraphicsFilter& aFilter,
                              const gfxMatrix& aTransform = gfxMatrix())
    {
        return mDrawable->Draw(aContext, aFillRect, false, aFilter,
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

bool
gfxPatternDrawable::Draw(gfxContext* aContext,
                         const gfxRect& aFillRect,
                         bool aRepeat,
                         const gfxPattern::GraphicsFilter& aFilter,
                         const gfxMatrix& aTransform)
{
    if (!mPattern)
        return false;

    if (aRepeat) {
        
        
        
        
        
        
        
        nsRefPtr<gfxCallbackDrawable> callbackDrawable = MakeCallbackDrawable();
        return callbackDrawable->Draw(aContext, aFillRect, true, aFilter,
                                      aTransform);
    }

    aContext->NewPath();
    gfxMatrix oldMatrix = mPattern->GetMatrix();
    mPattern->SetMatrix(gfxMatrix(aTransform).Multiply(oldMatrix));
    aContext->SetPattern(mPattern);
    aContext->Rectangle(aFillRect);
    aContext->Fill();
    mPattern->SetMatrix(oldMatrix);
    return true;
}
