




































#include "gfxCachedTempSurface.h"
#include "gfxContext.h"

class CachedSurfaceExpirationTracker :
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
      sExpirationTracker = nsnull;
    }
  }

private:
  static CachedSurfaceExpirationTracker* sExpirationTracker;
};

CachedSurfaceExpirationTracker*
CachedSurfaceExpirationTracker::sExpirationTracker = nsnull;

gfxCachedTempSurface::~gfxCachedTempSurface()
{
  CachedSurfaceExpirationTracker::RemoveSurface(this);
}

already_AddRefed<gfxContext>
gfxCachedTempSurface::Get(gfxASurface::gfxContentType aContentType,
                          const gfxRect& aRect,
                          gfxASurface* aSimilarTo)
{
  if (mSurface) {
    
    if (mSize.width < aRect.width || mSize.height < aRect.height
        || mSurface->GetContentType() != aContentType) {
      mSurface = nsnull;
    } else {
      NS_ASSERTION(mType == aSimilarTo->GetType(),
                   "Unexpected surface type change");
    }
  }

  bool cleared = false;
  if (!mSurface) {
    mSize = gfxIntSize(PRInt32(ceil(aRect.width)), PRInt32(ceil(aRect.height)));
    mSurface = aSimilarTo->CreateSimilarSurface(aContentType, mSize);
    if (!mSurface)
      return nsnull;

    cleared = true;
#ifdef DEBUG
    mType = aSimilarTo->GetType();
#endif
  }
  mSurface->SetDeviceOffset(-aRect.TopLeft());

  nsRefPtr<gfxContext> ctx = new gfxContext(mSurface);
  ctx->Rectangle(aRect);
  ctx->Clip();
  if (!cleared && aContentType != gfxASurface::CONTENT_COLOR) {
    ctx->SetOperator(gfxContext::OPERATOR_CLEAR);
    ctx->Paint();
    ctx->SetOperator(gfxContext::OPERATOR_OVER);
  }

  CachedSurfaceExpirationTracker::MarkSurfaceUsed(this);

  return ctx.forget();
}
