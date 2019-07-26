




#include "gfxCachedTempSurface.h"
#include "gfxContext.h"
#include "mozilla/Attributes.h"

class CachedSurfaceExpirationTracker MOZ_FINAL :
  public nsExpirationTracker<gfxCachedTempSurface,2> {

public:
  
  
  enum { TIMEOUT_MS = 1000 };
  CachedSurfaceExpirationTracker()
    : nsExpirationTracker<gfxCachedTempSurface,2>(TIMEOUT_MS) {}

  ~CachedSurfaceExpirationTracker() {
    AgeAllGenerations();
  }

  virtual void NotifyExpired(gfxCachedTempSurface* aSurface) {
    RemoveObject(aSurface);
    aSurface->Expire();
  }

  static void MarkSurfaceUsed(gfxCachedTempSurface* aSurface)
  {
    if (aSurface->GetExpirationState()->IsTracked()) {
      sExpirationTracker->MarkUsed(aSurface);
      return;
    }

    if (!sExpirationTracker) {
      sExpirationTracker = new CachedSurfaceExpirationTracker();
    }
    sExpirationTracker->AddObject(aSurface);
  }

  static void RemoveSurface(gfxCachedTempSurface* aSurface)
  {
    if (!sExpirationTracker)
      return;

    if (aSurface->GetExpirationState()->IsTracked()) {
      sExpirationTracker->RemoveObject(aSurface);
    }
    if (sExpirationTracker->IsEmpty()) {
      delete sExpirationTracker;
      sExpirationTracker = nullptr;
    }
  }

private:
  static CachedSurfaceExpirationTracker* sExpirationTracker;
};

CachedSurfaceExpirationTracker*
CachedSurfaceExpirationTracker::sExpirationTracker = nullptr;

gfxCachedTempSurface::~gfxCachedTempSurface()
{
  CachedSurfaceExpirationTracker::RemoveSurface(this);
}

already_AddRefed<gfxContext>
gfxCachedTempSurface::Get(gfxContentType aContentType,
                          const gfxRect& aRect,
                          gfxASurface* aSimilarTo)
{
  if (mSurface) {
    
    if (mSize.width < aRect.width || mSize.height < aRect.height
        || mSurface->GetContentType() != aContentType
        || mType != aSimilarTo->GetType()) {
      mSurface = nullptr;
    }
  }

  bool cleared = false;
  if (!mSurface) {
    mSize = gfxIntSize(int32_t(ceil(aRect.width)), int32_t(ceil(aRect.height)));
    mSurface = aSimilarTo->CreateSimilarSurface(aContentType, mSize);
    if (!mSurface)
      return nullptr;

    cleared = true;
    mType = aSimilarTo->GetType();
  }
  mSurface->SetDeviceOffset(-aRect.TopLeft());

  nsRefPtr<gfxContext> ctx = new gfxContext(mSurface);
  ctx->Rectangle(aRect);
  ctx->Clip();
  if (!cleared && aContentType != GFX_CONTENT_COLOR) {
    ctx->SetOperator(gfxContext::OPERATOR_CLEAR);
    ctx->Paint();
    ctx->SetOperator(gfxContext::OPERATOR_OVER);
  }

  CachedSurfaceExpirationTracker::MarkSurfaceUsed(this);

  return ctx.forget();
}
