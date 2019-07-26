




#ifndef GFX_CACHED_TEMP_SURFACE_H
#define GFX_CACHED_TEMP_SURFACE_H

#include "gfxASurface.h"
#include "nsExpirationTracker.h"

class gfxContext;












class THEBES_API gfxCachedTempSurface {
public:
  













  already_AddRefed<gfxContext> Get(gfxASurface::gfxContentType aContentType,
                                   const gfxRect& aRect,
                                   gfxASurface* aSimilarTo);

  void Expire() { mSurface = nullptr; }
  nsExpirationState* GetExpirationState() { return &mExpirationState; }
  ~gfxCachedTempSurface();

  bool IsSurface(gfxASurface* aSurface) { return mSurface == aSurface; }

private:
  nsRefPtr<gfxASurface> mSurface;
  gfxIntSize mSize;
  nsExpirationState mExpirationState;
#ifdef DEBUG
  gfxASurface::gfxSurfaceType mType;
#endif 
};

#endif 
