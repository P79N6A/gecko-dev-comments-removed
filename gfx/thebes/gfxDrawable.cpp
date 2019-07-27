




#include "gfxDrawable.h"
#include "gfxASurface.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxColor.h"
#include "gfx2DGlue.h"
#ifdef MOZ_X11
#include "cairo.h"
#include "gfxXlibSurface.h"
#endif
#include "mozilla/gfx/Logging.h"

using namespace mozilla;
using namespace mozilla::gfx;

gfxSurfaceDrawable::gfxSurfaceDrawable(SourceSurface* aSurface,
                                       const gfxIntSize aSize,
                                       const gfxMatrix aTransform)
 : gfxDrawable(aSize)
 , mSourceSurface(aSurface)
 , mTransform(aTransform)
{
  if (!mSourceSurface) {
    gfxWarning() << "Creating gfxSurfaceDrawable with null SourceSurface";
  }
}

bool
gfxSurfaceDrawable::DrawWithSamplingRect(gfxContext* aContext,
                                         const gfxRect& aFillRect,
                                         const gfxRect& aSamplingRect,
                                         bool aRepeat,
                                         const GraphicsFilter& aFilter,
                                         gfxFloat aOpacity)
{
  if (!mSourceSurface) {
    return true;
  }

  
  
  gfxRect samplingRect = aSamplingRect;
  samplingRect.RoundOut();
  IntRect intRect(samplingRect.x, samplingRect.y, samplingRect.width, samplingRect.height);

  IntSize size = mSourceSurface->GetSize();
  if (!IntRect(0, 0, size.width, size.height).Contains(intRect)) {
    return false;
  }

  DrawInternal(aContext, aFillRect, intRect, false, aFilter, aOpacity, gfxMatrix());
  return true;
}

bool
gfxSurfaceDrawable::Draw(gfxContext* aContext,
                         const gfxRect& aFillRect,
                         bool aRepeat,
                         const GraphicsFilter& aFilter,
                         gfxFloat aOpacity,
                         const gfxMatrix& aTransform)
{
  if (!mSourceSurface) {
    return true;
  }

  DrawInternal(aContext, aFillRect, IntRect(), aRepeat, aFilter, aOpacity, aTransform);
  return true;
}

void
gfxSurfaceDrawable::DrawInternal(gfxContext* aContext,
                                 const gfxRect& aFillRect,
                                 const IntRect& aSamplingRect,
                                 bool aRepeat,
                                 const GraphicsFilter& aFilter,
                                 gfxFloat aOpacity,
                                 const gfxMatrix& aTransform)
{
    ExtendMode extend = ExtendMode::CLAMP;

    if (aRepeat) {
        extend = ExtendMode::REPEAT;
    }

    Matrix patternTransform = ToMatrix(aTransform * mTransform);
    patternTransform.Invert();

    SurfacePattern pattern(mSourceSurface, extend,
                           patternTransform, ToFilter(aFilter), aSamplingRect);

    Rect fillRect = ToRect(aFillRect);
    DrawTarget* dt = aContext->GetDrawTarget();

    if (aContext->CurrentOperator() == gfxContext::OPERATOR_SOURCE &&
        aOpacity == 1.0) {
        
        dt->ClearRect(fillRect);
        dt->FillRect(fillRect, pattern);
    } else {
        dt->FillRect(fillRect, pattern,
                     DrawOptions(aOpacity,
                                 CompositionOpForOp(aContext->CurrentOperator()),
                                 aContext->CurrentAntialiasMode()));
    }
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
    SurfaceFormat format =
        gfxPlatform::GetPlatform()->Optimal2DFormatForContent(gfxContentType::COLOR_ALPHA);
    RefPtr<DrawTarget> dt =
        gfxPlatform::GetPlatform()->CreateOffscreenContentDrawTarget(mSize,
                                                                     format);
    if (!dt)
        return nullptr;

    nsRefPtr<gfxContext> ctx = new gfxContext(dt);
    Draw(ctx, gfxRect(0, 0, mSize.width, mSize.height), false, aFilter);

    RefPtr<SourceSurface> surface = dt->Snapshot();
    if (surface) {
        nsRefPtr<gfxSurfaceDrawable> drawable = new gfxSurfaceDrawable(surface, mSize);
        return drawable.forget();
    }
    return nullptr;
}

bool
gfxCallbackDrawable::Draw(gfxContext* aContext,
                          const gfxRect& aFillRect,
                          bool aRepeat,
                          const GraphicsFilter& aFilter,
                          gfxFloat aOpacity,
                          const gfxMatrix& aTransform)
{
    if ((aRepeat || aOpacity != 1.0) && !mSurfaceDrawable) {
        mSurfaceDrawable = MakeSurfaceDrawable(aFilter);
    }

    if (mSurfaceDrawable)
        return mSurfaceDrawable->Draw(aContext, aFillRect, aRepeat, aFilter,
                                      aOpacity, aTransform);

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
    explicit DrawingCallbackFromDrawable(gfxDrawable* aDrawable)
     : mDrawable(aDrawable) {
        NS_ASSERTION(aDrawable, "aDrawable is null!");
    }

    virtual ~DrawingCallbackFromDrawable() {}

    virtual bool operator()(gfxContext* aContext,
                              const gfxRect& aFillRect,
                              const GraphicsFilter& aFilter,
                              const gfxMatrix& aTransform = gfxMatrix())
    {
        return mDrawable->Draw(aContext, aFillRect, false, aFilter, 1.0,
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
                         gfxFloat aOpacity,
                         const gfxMatrix& aTransform)
{
    DrawTarget& aDrawTarget = *aContext->GetDrawTarget();

    if (!mPattern)
        return false;

    if (aRepeat) {
        
        
        
        
        
        
        
        nsRefPtr<gfxCallbackDrawable> callbackDrawable = MakeCallbackDrawable();
        return callbackDrawable->Draw(aContext, aFillRect, true, aFilter,
                                      aOpacity, aTransform);
    }

    gfxMatrix oldMatrix = mPattern->GetMatrix();
    mPattern->SetMatrix(aTransform * oldMatrix);
    DrawOptions drawOptions(aOpacity);
    aDrawTarget.FillRect(ToRect(aFillRect),
                         *mPattern->GetPattern(&aDrawTarget), drawOptions);
    mPattern->SetMatrix(oldMatrix);
    return true;
}
