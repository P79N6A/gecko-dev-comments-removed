




































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
                          const gfxIntSize& aSize,
                          gfxASurface* aSimilarTo)
{
  if (mSurface) {
    
    if (mSize.width < aSize.width || mSize.height < aSize.height
        || mSurface->GetContentType() != aContentType) {
      mSurface = nsnull;
    } else {
      NS_ASSERTION(mType == aSimilarTo->GetType(),
                   "Unexpected surface type change");
    }
  }

  PRBool cleared = PR_FALSE;
  if (!mSurface) {
    mSize = aSize;
    mSurface = aSimilarTo->CreateSimilarSurface(aContentType, aSize);
    if (!mSurface)
      return nsnull;

    cleared = PR_TRUE;
#ifdef DEBUG
    mType = aSimilarTo->GetType();
#endif
  }

  nsRefPtr<gfxContext> ctx = new gfxContext(mSurface);
  ctx->Rectangle(gfxRect(0, 0, aSize.width, aSize.height));
  ctx->Clip();
  if (!cleared && aContentType != gfxASurface::CONTENT_COLOR) {
    ctx->SetOperator(gfxContext::OPERATOR_CLEAR);
    ctx->Paint();
    ctx->SetOperator(gfxContext::OPERATOR_OVER);
  }

  CachedSurfaceExpirationTracker::MarkSurfaceUsed(this);

  return ctx.forget();
}
