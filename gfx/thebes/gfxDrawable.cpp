




#include "gfxDrawable.h"
#include "gfxASurface.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxColor.h"
#ifdef MOZ_X11
#include "cairo.h"
#include "gfxXlibSurface.h"
#endif

using namespace mozilla;
using namespace mozilla::gfx;

gfxSurfaceDrawable::gfxSurfaceDrawable(gfxASurface* aSurface,
                                       const gfxIntSize aSize,
                                       const gfxMatrix aTransform)
 : gfxDrawable(aSize)
 , mSurface(aSurface)
 , mTransform(aTransform)
{
}

gfxSurfaceDrawable::gfxSurfaceDrawable(DrawTarget* aDrawTarget,
                                       const gfxIntSize aSize,
                                       const gfxMatrix aTransform)
 : gfxDrawable(aSize)
 , mDrawTarget(aDrawTarget)
 , mTransform(aTransform)
{
}

gfxSurfaceDrawable::gfxSurfaceDrawable(SourceSurface* aSurface,
                                       const gfxIntSize aSize,
                                       const gfxMatrix aTransform)
 : gfxDrawable(aSize)
 , mSourceSurface(aSurface)
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
                                const GraphicsFilter aDefaultFilter)
{
    if (!currentTarget) {
        
        aPattern->SetExtend(gfxPattern::EXTEND_PAD);
        aPattern->SetFilter(aDefaultFilter);
        return;
    }

    
    
    
    switch (currentTarget->GetType()) {

#ifdef MOZ_X11
        case gfxSurfaceType::Xlib:
        {
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            
            if (static_cast<gfxXlibSurface*>(currentTarget)->IsPadSlow()) {
                bool isDownscale =
                    aDeviceToImage.xx >= 1.0 && aDeviceToImage.yy >= 1.0 &&
                    aDeviceToImage.xy == 0.0 && aDeviceToImage.yx == 0.0;

                GraphicsFilter filter =
                    isDownscale ? aDefaultFilter : (const GraphicsFilter)GraphicsFilter::FILTER_FAST;
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
                         const GraphicsFilter& aFilter,
                         const gfxMatrix& aTransform)
{
    nsRefPtr<gfxPattern> pattern;
    if (mDrawTarget) {
      if (aContext->IsCairo()) {
        nsRefPtr<gfxASurface> source =
          gfxPlatform::GetPlatform()->GetThebesSurfaceForDrawTarget(mDrawTarget);
        pattern = new gfxPattern(source);
      } else {
        RefPtr<SourceSurface> source = mDrawTarget->Snapshot();
        pattern = new gfxPattern(source, Matrix());
      }
    } else if (mSourceSurface) {
      pattern = new gfxPattern(mSourceSurface, Matrix());
    } else {
      pattern = new gfxPattern(mSurface);
    }
    if (aRepeat) {
        pattern->SetExtend(gfxPattern::EXTEND_REPEAT);
        pattern->SetFilter(aFilter);
    } else {
        GraphicsFilter filter = aFilter;
        if (aContext->CurrentMatrix().HasOnlyIntegerTranslation() &&
            aTransform.HasOnlyIntegerTranslation())
        {
          
          
          
          filter = GraphicsFilter::FILTER_FAST;
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
    
    
    aContext->SetDeviceColor(gfxRGBA(0.0, 0.0, 0.0, 0.0));
    return true;
}

already_AddRefed<gfxImageSurface>
gfxSurfaceDrawable::GetAsImageSurface()
{
    if (mDrawTarget || mSourceSurface) {
      
      
      
      return nullptr;

    }
    return mSurface->GetAsImageSurface();
}

gfxCallbackDrawable::gfxCallbackDrawable(gfxDrawingCallback* aCallback,
                                         const gfxIntSize aSize)
 : gfxDrawable(aSize)
 , mCallback(aCallback)
{
}

already_AddRefed<gfxSurfaceDrawable>
gfxCallbackDrawable::MakeSurfaceDrawable(const GraphicsFilter aFilter)
{
    nsRefPtr<gfxASurface> surface =
        gfxPlatform::GetPlatform()->CreateOffscreenSurface(mSize, GFX_CONTENT_COLOR_ALPHA);
    if (!surface || surface->CairoStatus() != 0)
        return nullptr;

    nsRefPtr<gfxContext> ctx = new gfxContext(surface);
    Draw(ctx, gfxRect(0, 0, mSize.width, mSize.height), false, aFilter);
    nsRefPtr<gfxSurfaceDrawable> drawable = new gfxSurfaceDrawable(surface, mSize);
    return drawable.forget();
}

bool
gfxCallbackDrawable::Draw(gfxContext* aContext,
                          const gfxRect& aFillRect,
                          bool aRepeat,
                          const GraphicsFilter& aFilter,
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

gfxPatternDrawable::~gfxPatternDrawable()
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
                              const GraphicsFilter& aFilter,
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
                         const GraphicsFilter& aFilter,
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
