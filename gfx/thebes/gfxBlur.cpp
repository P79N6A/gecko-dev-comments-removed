




#include "gfxBlur.h"

#include "gfx2DGlue.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/Blur.h"
#include "mozilla/gfx/PathHelpers.h"
#include "mozilla/UniquePtr.h"
#include "nsExpirationTracker.h"
#include "nsClassHashtable.h"
#include "gfxUtils.h"

using namespace mozilla;
using namespace mozilla::gfx;

gfxAlphaBoxBlur::gfxAlphaBoxBlur()
{
}

gfxAlphaBoxBlur::~gfxAlphaBoxBlur()
{
  mContext = nullptr;
}

gfxContext*
gfxAlphaBoxBlur::Init(const gfxRect& aRect,
                      const IntSize& aSpreadRadius,
                      const IntSize& aBlurRadius,
                      const gfxRect* aDirtyRect,
                      const gfxRect* aSkipRect)
{
    mozilla::gfx::Rect rect(Float(aRect.x), Float(aRect.y),
                            Float(aRect.width), Float(aRect.height));
    IntSize spreadRadius(aSpreadRadius.width, aSpreadRadius.height);
    IntSize blurRadius(aBlurRadius.width, aBlurRadius.height);
    UniquePtr<Rect> dirtyRect;
    if (aDirtyRect) {
      dirtyRect = MakeUnique<Rect>(Float(aDirtyRect->x),
                                   Float(aDirtyRect->y),
                                   Float(aDirtyRect->width),
                                   Float(aDirtyRect->height));
    }
    UniquePtr<Rect> skipRect;
    if (aSkipRect) {
      skipRect = MakeUnique<Rect>(Float(aSkipRect->x),
                                  Float(aSkipRect->y),
                                  Float(aSkipRect->width),
                                  Float(aSkipRect->height));
    }

    mBlur = MakeUnique<AlphaBoxBlur>(rect, spreadRadius, blurRadius, dirtyRect.get(), skipRect.get());
    size_t blurDataSize = mBlur->GetSurfaceAllocationSize();
    if (blurDataSize == 0)
        return nullptr;

    IntSize size = mBlur->GetSize();

    
    
    mData = new (std::nothrow) unsigned char[blurDataSize];
    if (!mData) {
        return nullptr;
    }
    memset(mData, 0, blurDataSize);

    mozilla::RefPtr<DrawTarget> dt =
        gfxPlatform::GetPlatform()->CreateDrawTargetForData(mData, size,
                                                            mBlur->GetStride(),
                                                            SurfaceFormat::A8);
    if (!dt) {
        return nullptr;
    }

    IntRect irect = mBlur->GetRect();
    gfxPoint topleft(irect.TopLeft().x, irect.TopLeft().y);

    mContext = new gfxContext(dt);
    mContext->SetMatrix(gfxMatrix::Translation(-topleft));

    return mContext;
}

void
DrawBlur(gfxContext* aDestinationCtx,
         SourceSurface* aBlur,
         const IntPoint& aTopLeft,
         const Rect* aDirtyRect)
{
    DrawTarget *dest = aDestinationCtx->GetDrawTarget();

    nsRefPtr<gfxPattern> thebesPat = aDestinationCtx->GetPattern();
    Pattern* pat = thebesPat->GetPattern(dest, nullptr);

    Matrix oldTransform = dest->GetTransform();
    Matrix newTransform = oldTransform;
    newTransform.PreTranslate(aTopLeft.x, aTopLeft.y);

    
    
    if (aDirtyRect) {
        dest->PushClipRect(*aDirtyRect);
    }

    dest->SetTransform(newTransform);
    dest->MaskSurface(*pat, aBlur, Point(0, 0));
    dest->SetTransform(oldTransform);

    if (aDirtyRect) {
        dest->PopClip();
    }
}

TemporaryRef<SourceSurface>
gfxAlphaBoxBlur::DoBlur(DrawTarget* aDT, IntPoint* aTopLeft)
{
    mBlur->Blur(mData);

    *aTopLeft = mBlur->GetRect().TopLeft();

    return aDT->CreateSourceSurfaceFromData(mData,
                                            mBlur->GetSize(),
                                            mBlur->GetStride(),
                                            SurfaceFormat::A8);
}

void
gfxAlphaBoxBlur::Paint(gfxContext* aDestinationCtx)
{
    if (!mContext)
        return;

    DrawTarget *dest = aDestinationCtx->GetDrawTarget();
    if (!dest) {
      NS_WARNING("Blurring not supported for Thebes contexts!");
      return;
    }

    Rect* dirtyRect = mBlur->GetDirtyRect();

    IntPoint topLeft;
    RefPtr<SourceSurface> mask = DoBlur(dest, &topLeft);
    if (!mask) {
      NS_ERROR("Failed to create mask!");
      return;
    }

    DrawBlur(aDestinationCtx, mask, topLeft, dirtyRect);
}

IntSize gfxAlphaBoxBlur::CalculateBlurRadius(const gfxPoint& aStd)
{
    mozilla::gfx::Point std(Float(aStd.x), Float(aStd.y));
    IntSize size = AlphaBoxBlur::CalculateBlurRadius(std);
    return IntSize(size.width, size.height);
}

struct BlurCacheKey : public PLDHashEntryHdr {
  typedef const BlurCacheKey& KeyType;
  typedef const BlurCacheKey* KeyTypePointer;
  enum { ALLOW_MEMMOVE = true };

  gfxRect mRect;
  IntSize mBlurRadius;
  gfxRect mSkipRect;
  BackendType mBackend;

  BlurCacheKey(const gfxRect& aRect, const IntSize &aBlurRadius, const gfxRect& aSkipRect, BackendType aBackend)
    : mRect(aRect)
    , mBlurRadius(aBlurRadius)
    , mSkipRect(aSkipRect)
    , mBackend(aBackend)
  { }

  explicit BlurCacheKey(const BlurCacheKey* aOther)
    : mRect(aOther->mRect)
    , mBlurRadius(aOther->mBlurRadius)
    , mSkipRect(aOther->mSkipRect)
    , mBackend(aOther->mBackend)
  { }

  static PLDHashNumber
  HashKey(const KeyTypePointer aKey)
  {
    PLDHashNumber hash = HashBytes(&aKey->mRect.x, 4 * sizeof(gfxFloat));
    hash = AddToHash(hash, aKey->mBlurRadius.width, aKey->mBlurRadius.height);
    hash = AddToHash(hash, HashBytes(&aKey->mSkipRect.x, 4 * sizeof(gfxFloat)));
    hash = AddToHash(hash, (uint32_t)aKey->mBackend);
    return hash;
  }

  bool KeyEquals(KeyTypePointer aKey) const
  {
    if (aKey->mRect.IsEqualInterior(mRect) &&
        aKey->mBlurRadius == mBlurRadius &&
        aKey->mSkipRect.IsEqualInterior(mSkipRect) &&
        aKey->mBackend == mBackend) {
      return true;
    }
    return false;
  }
  static KeyTypePointer KeyToPointer(KeyType aKey)
  {
    return &aKey;
  }
};





struct BlurCacheData {
  BlurCacheData(SourceSurface* aBlur, const IntPoint& aTopLeft, const gfxRect& aDirtyRect, const BlurCacheKey& aKey)
    : mBlur(aBlur)
    , mTopLeft(aTopLeft)
    , mDirtyRect(aDirtyRect)
    , mKey(aKey)
  {}

  BlurCacheData(const BlurCacheData& aOther)
    : mBlur(aOther.mBlur)
    , mTopLeft(aOther.mTopLeft)
    , mDirtyRect(aOther.mDirtyRect)
    , mKey(aOther.mKey)
  { }

  nsExpirationState *GetExpirationState() {
    return &mExpirationState;
  }

  nsExpirationState mExpirationState;
  RefPtr<SourceSurface> mBlur;
  IntPoint mTopLeft;
  gfxRect mDirtyRect;
  BlurCacheKey mKey;
};







class BlurCache final : public nsExpirationTracker<BlurCacheData,4>
{
  public:
    BlurCache()
      : nsExpirationTracker<BlurCacheData, 4>(GENERATION_MS)
    {
    }

    virtual void NotifyExpired(BlurCacheData* aObject)
    {
      RemoveObject(aObject);
      mHashEntries.Remove(aObject->mKey);
    }

    BlurCacheData* Lookup(const gfxRect& aRect,
                          const IntSize& aBlurRadius,
                          const gfxRect& aSkipRect,
                          BackendType aBackendType,
                          const gfxRect* aDirtyRect)
    {
      BlurCacheData* blur =
        mHashEntries.Get(BlurCacheKey(aRect, aBlurRadius, aSkipRect, aBackendType));

      if (blur) {
        if (aDirtyRect && !blur->mDirtyRect.Contains(*aDirtyRect)) {
          return nullptr;
        }
        MarkUsed(blur);
      }

      return blur;
    }

    
    
    bool RegisterEntry(BlurCacheData* aValue)
    {
      nsresult rv = AddObject(aValue);
      if (NS_FAILED(rv)) {
        
        
        
        
        
        return false;
      }
      mHashEntries.Put(aValue->mKey, aValue);
      return true;
    }

  protected:
    static const uint32_t GENERATION_MS = 1000;
    



    nsClassHashtable<BlurCacheKey, BlurCacheData> mHashEntries;
};

static BlurCache* gBlurCache = nullptr;

SourceSurface*
GetCachedBlur(DrawTarget *aDT,
              const gfxRect& aRect,
              const IntSize& aBlurRadius,
              const gfxRect& aSkipRect,
              const gfxRect& aDirtyRect,
              IntPoint* aTopLeft)
{
  if (!gBlurCache) {
    gBlurCache = new BlurCache();
  }
  BlurCacheData* cached = gBlurCache->Lookup(aRect, aBlurRadius, aSkipRect,
                                             aDT->GetBackendType(),
                                             &aDirtyRect);
  if (cached) {
    *aTopLeft = cached->mTopLeft;
    return cached->mBlur;
  }
  return nullptr;
}

void
CacheBlur(DrawTarget *aDT,
          const gfxRect& aRect,
          const IntSize& aBlurRadius,
          const gfxRect& aSkipRect,
          SourceSurface* aBlur,
          const IntPoint& aTopLeft,
          const gfxRect& aDirtyRect)
{
  
  
  if (BlurCacheData* cached = gBlurCache->Lookup(aRect, aBlurRadius, aSkipRect,
                                                 aDT->GetBackendType(),
                                                 nullptr)) {
    cached->mBlur = aBlur;
    cached->mTopLeft = aTopLeft;
    cached->mDirtyRect = aDirtyRect;
    return;
  }

  BlurCacheKey key(aRect, aBlurRadius, aSkipRect, aDT->GetBackendType());
  BlurCacheData* data = new BlurCacheData(aBlur, aTopLeft, aDirtyRect, key);
  if (!gBlurCache->RegisterEntry(data)) {
    delete data;
  }
}

void
gfxAlphaBoxBlur::ShutdownBlurCache()
{
  delete gBlurCache;
  gBlurCache = nullptr;
}

static IntSize
ComputeMinimalSizeForShadowShape(RectCornerRadii* aCornerRadii,
                                 gfxIntSize aBlurRadius,
                                 IntMargin& aSlice)
{
  float cornerWidth = 0;
  float cornerHeight = 0;
  if (aCornerRadii) {
    RectCornerRadii corners = *aCornerRadii;
    for (size_t i = 0; i < 4; i++) {
      cornerWidth = std::max(cornerWidth, corners[i].width);
      cornerHeight = std::max(cornerHeight, corners[i].height);
    }
  }

  aSlice = IntMargin(ceil(cornerHeight) + aBlurRadius.height,
                     ceil(cornerWidth) + aBlurRadius.width,
                     ceil(cornerHeight) + aBlurRadius.height,
                     ceil(cornerWidth) + aBlurRadius.width);

  
  return IntSize(aSlice.LeftRight() + 1,
                 aSlice.TopBottom() + 1);
}


static TemporaryRef<SourceSurface>
CreateBlurMask(const IntSize& aRectSize,
               RectCornerRadii* aCornerRadii,
               gfxIntSize aBlurRadius,
               IntMargin& aExtendDestBy,
               IntMargin& aSliceBorder,
               DrawTarget& aDestDrawTarget)
{
  IntMargin slice;
  IntSize minimalSize =
    ComputeMinimalSizeForShadowShape(aCornerRadii, aBlurRadius, slice);

  
  
  
  
  
  
  if (aRectSize.width < minimalSize.width) {
    minimalSize.width = aRectSize.width;
    slice.left = 0;
    slice.right = 0;
  }
  if (aRectSize.height < minimalSize.height) {
    minimalSize.height = aRectSize.height;
    slice.top = 0;
    slice.bottom = 0;
  }

  MOZ_ASSERT(slice.LeftRight() <= minimalSize.width);
  MOZ_ASSERT(slice.TopBottom() <= minimalSize.height);

  IntRect minimalRect(IntPoint(), minimalSize);

  gfxAlphaBoxBlur blur;
  gfxContext* blurCtx = blur.Init(ThebesRect(Rect(minimalRect)), gfxIntSize(),
                                  aBlurRadius, nullptr, nullptr);
  if (!blurCtx) {
    return nullptr;
  }

  DrawTarget* blurDT = blurCtx->GetDrawTarget();
  ColorPattern black(Color(0.f, 0.f, 0.f, 1.f));

  if (aCornerRadii) {
    RefPtr<Path> roundedRect =
      MakePathForRoundedRect(*blurDT, Rect(minimalRect), *aCornerRadii);
    blurDT->Fill(roundedRect, black);
  } else {
    blurDT->FillRect(Rect(minimalRect), black);
  }

  IntPoint topLeft;
  RefPtr<SourceSurface> result = blur.DoBlur(&aDestDrawTarget, &topLeft);

  IntRect expandedMinimalRect(topLeft, result->GetSize());
  aExtendDestBy = expandedMinimalRect - minimalRect;
  aSliceBorder = slice + aExtendDestBy;

  MOZ_ASSERT(aSliceBorder.LeftRight() <= expandedMinimalRect.width);
  MOZ_ASSERT(aSliceBorder.TopBottom() <= expandedMinimalRect.height);

  return result.forget();
}

static TemporaryRef<SourceSurface>
CreateBoxShadow(DrawTarget& aDT, SourceSurface* aBlurMask, const gfxRGBA& aShadowColor)
{
  IntSize blurredSize = aBlurMask->GetSize();
  gfxPlatform* platform = gfxPlatform::GetPlatform();
  RefPtr<DrawTarget> boxShadowDT =
    platform->CreateOffscreenContentDrawTarget(blurredSize, SurfaceFormat::B8G8R8A8);
  MOZ_ASSERT(boxShadowDT->GetType() == aDT.GetType());

  ColorPattern shadowColor(ToDeviceColor(aShadowColor));
  boxShadowDT->MaskSurface(shadowColor, aBlurMask, Point(0, 0));
  return boxShadowDT->Snapshot();
}

static Rect
RectWithEdgesTRBL(Float aTop, Float aRight, Float aBottom, Float aLeft)
{
  return Rect(aLeft, aTop, aRight - aLeft, aBottom - aTop);
}

static void
RepeatOrStretchSurface(DrawTarget& aDT, SourceSurface* aSurface,
                       const Rect& aDest, const Rect& aSrc, Rect& aSkipRect)
{
  if (aSkipRect.Contains(aDest)) {
    return;
  }

  if ((!aDT.GetTransform().IsRectilinear() &&
      aDT.GetBackendType() != BackendType::CAIRO) ||
      (aDT.GetBackendType() == BackendType::DIRECT2D)) {
    
    
    
    
    
    
    
    aDT.DrawSurface(aSurface, aDest, aSrc);
    return;
  }

  SurfacePattern pattern(aSurface, ExtendMode::REPEAT,
                         Matrix::Translation(aDest.TopLeft() - aSrc.TopLeft()),
                         Filter::GOOD, RoundedToInt(aSrc));
  aDT.FillRect(aDest, pattern);
}

static void
DrawCorner(DrawTarget& aDT, SourceSurface* aSurface,
           const Rect& aDest, const Rect& aSrc, Rect& aSkipRect)
{
  if (aSkipRect.Contains(aDest)) {
    return;
  }

  aDT.DrawSurface(aSurface, aDest, aSrc);
}













 void
gfxAlphaBoxBlur::BlurRectangle(gfxContext *aDestinationCtx,
                               const gfxRect& aRect,
                               RectCornerRadii* aCornerRadii,
                               const gfxPoint& aBlurStdDev,
                               const gfxRGBA& aShadowColor,
                               const gfxRect& aDirtyRect,
                               const gfxRect& aSkipRect)
{
  DrawTarget& destDrawTarget = *aDestinationCtx->GetDrawTarget();
  IntSize blurRadius = CalculateBlurRadius(aBlurStdDev);

  IntRect rect = RoundedToInt(ToRect(aRect));
  IntMargin extendDestBy;
  IntMargin slice;
  RefPtr<SourceSurface> blurMask =
    CreateBlurMask(rect.Size(), aCornerRadii, blurRadius, extendDestBy, slice,
                   destDrawTarget);
  if (!blurMask) {
    return;
  }

  RefPtr<SourceSurface> boxShadow = CreateBoxShadow(destDrawTarget, blurMask, aShadowColor);

  destDrawTarget.PushClipRect(ToRect(aDirtyRect));

  
  

  Rect srcOuter(Point(), Size(boxShadow->GetSize()));
  Rect srcInner = srcOuter;
  srcInner.Deflate(Margin(slice));

  rect.Inflate(extendDestBy);
  Rect dstOuter(rect);
  Rect dstInner(rect);
  dstInner.Deflate(Margin(slice));

  Rect skipRect = ToRect(aSkipRect);

  if (srcInner.IsEqualInterior(srcOuter)) {
    MOZ_ASSERT(dstInner.IsEqualInterior(dstOuter));
    
    destDrawTarget.DrawSurface(boxShadow, dstInner, srcInner);
  } else {
    
    DrawCorner(destDrawTarget, boxShadow,
               RectWithEdgesTRBL(dstOuter.Y(), dstInner.X(),
                                 dstInner.Y(), dstOuter.X()),
               RectWithEdgesTRBL(srcOuter.Y(), srcInner.X(),
                                 srcInner.Y(), srcOuter.X()),
               skipRect);

    DrawCorner(destDrawTarget, boxShadow,
               RectWithEdgesTRBL(dstOuter.Y(), dstOuter.XMost(),
                                 dstInner.Y(), dstInner.XMost()),
               RectWithEdgesTRBL(srcOuter.Y(), srcOuter.XMost(),
                                 srcInner.Y(), srcInner.XMost()),
               skipRect);

    DrawCorner(destDrawTarget, boxShadow,
               RectWithEdgesTRBL(dstInner.YMost(), dstInner.X(),
                                 dstOuter.YMost(), dstOuter.X()),
               RectWithEdgesTRBL(srcInner.YMost(), srcInner.X(),
                                 srcOuter.YMost(), srcOuter.X()),
               skipRect);

    DrawCorner(destDrawTarget, boxShadow,
               RectWithEdgesTRBL(dstInner.YMost(), dstOuter.XMost(),
                                 dstOuter.YMost(), dstInner.XMost()),
               RectWithEdgesTRBL(srcInner.YMost(), srcOuter.XMost(),
                                 srcOuter.YMost(), srcInner.XMost()),
               skipRect);

    
    RepeatOrStretchSurface(destDrawTarget, boxShadow,
                           RectWithEdgesTRBL(dstOuter.Y(), dstInner.XMost(),
                                             dstInner.Y(), dstInner.X()),
                           RectWithEdgesTRBL(srcOuter.Y(), srcInner.XMost(),
                                             srcInner.Y(), srcInner.X()),
                           skipRect);
    RepeatOrStretchSurface(destDrawTarget, boxShadow,
                           RectWithEdgesTRBL(dstInner.Y(), dstInner.X(),
                                             dstInner.YMost(), dstOuter.X()),
                           RectWithEdgesTRBL(srcInner.Y(), srcInner.X(),
                                             srcInner.YMost(), srcOuter.X()),
                           skipRect);
    RepeatOrStretchSurface(destDrawTarget, boxShadow,
                           RectWithEdgesTRBL(dstInner.Y(), dstOuter.XMost(),
                                             dstInner.YMost(), dstInner.XMost()),
                           RectWithEdgesTRBL(srcInner.Y(), srcOuter.XMost(),
                                             srcInner.YMost(), srcInner.XMost()),
                           skipRect);
    RepeatOrStretchSurface(destDrawTarget, boxShadow,
                           RectWithEdgesTRBL(dstInner.YMost(), dstInner.XMost(),
                                             dstOuter.YMost(), dstInner.X()),
                           RectWithEdgesTRBL(srcInner.YMost(), srcInner.XMost(),
                                             srcOuter.YMost(), srcInner.X()),
                           skipRect);

    
    RepeatOrStretchSurface(destDrawTarget, boxShadow,
                           RectWithEdgesTRBL(dstInner.Y(), dstInner.XMost(),
                                             dstInner.YMost(), dstInner.X()),
                           RectWithEdgesTRBL(srcInner.Y(), srcInner.XMost(),
                                             srcInner.YMost(), srcInner.X()),
                           skipRect);
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  destDrawTarget.PopClip();
}

