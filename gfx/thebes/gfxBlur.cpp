




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

already_AddRefed<SourceSurface>
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

  IntSize mMinSize;
  IntSize mBlurRadius;
  gfxRGBA mShadowColor;
  BackendType mBackend;
  RectCornerRadii mCornerRadii;

  BlurCacheKey(IntSize aMinimumSize, gfxIntSize aBlurRadius,
               RectCornerRadii* aCornerRadii, gfxRGBA aShadowColor,
               BackendType aBackend)
    : mMinSize(aMinimumSize)
    , mBlurRadius(aBlurRadius)
    , mShadowColor(aShadowColor)
    , mBackend(aBackend)
    , mCornerRadii(aCornerRadii ? *aCornerRadii : RectCornerRadii())
  { }

  explicit BlurCacheKey(const BlurCacheKey* aOther)
    : mMinSize(aOther->mMinSize)
    , mBlurRadius(aOther->mBlurRadius)
    , mShadowColor(aOther->mShadowColor)
    , mBackend(aOther->mBackend)
    , mCornerRadii(aOther->mCornerRadii)
  { }

  static PLDHashNumber
  HashKey(const KeyTypePointer aKey)
  {
    PLDHashNumber hash = 0;
    hash = AddToHash(hash, aKey->mMinSize.width, aKey->mMinSize.height);
    hash = AddToHash(hash, aKey->mBlurRadius.width, aKey->mBlurRadius.height);

    hash = AddToHash(hash, HashBytes(&aKey->mShadowColor.r, sizeof(gfxFloat)));
    hash = AddToHash(hash, HashBytes(&aKey->mShadowColor.g, sizeof(gfxFloat)));
    hash = AddToHash(hash, HashBytes(&aKey->mShadowColor.b, sizeof(gfxFloat)));
    hash = AddToHash(hash, HashBytes(&aKey->mShadowColor.a, sizeof(gfxFloat)));

    for (int i = 0; i < 4; i++) {
    hash = AddToHash(hash, aKey->mCornerRadii[i].width, aKey->mCornerRadii[i].height);
    }

    hash = AddToHash(hash, (uint32_t)aKey->mBackend);
    return hash;
  }

  bool KeyEquals(KeyTypePointer aKey) const
  {
    if (aKey->mMinSize == mMinSize &&
        aKey->mBlurRadius == mBlurRadius &&
        aKey->mCornerRadii == mCornerRadii &&
        aKey->mShadowColor == mShadowColor &&
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
  BlurCacheData(SourceSurface* aBlur, IntMargin aExtendDestBy, const BlurCacheKey& aKey)
    : mBlur(aBlur)
    , mExtendDest(aExtendDestBy)
    , mKey(aKey)
  {}

  BlurCacheData(const BlurCacheData& aOther)
    : mBlur(aOther.mBlur)
    , mExtendDest(aOther.mExtendDest)
    , mKey(aOther.mKey)
  { }

  nsExpirationState *GetExpirationState() {
    return &mExpirationState;
  }

  nsExpirationState mExpirationState;
  RefPtr<SourceSurface> mBlur;
  IntMargin mExtendDest;
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

    BlurCacheData* Lookup(const IntSize aMinSize,
                          const gfxIntSize& aBlurRadius,
                          RectCornerRadii* aCornerRadii,
                          const gfxRGBA& aShadowColor,
                          BackendType aBackendType)
    {
      BlurCacheData* blur =
        mHashEntries.Get(BlurCacheKey(aMinSize, aBlurRadius,
                                      aCornerRadii, aShadowColor,
                                      aBackendType));
      if (blur) {
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

static IntSize
ComputeMinSizeForShadowShape(RectCornerRadii* aCornerRadii,
                             gfxIntSize aBlurRadius,
                             IntMargin& aSlice,
                             const IntSize& aRectSize)
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

  IntSize minSize(aSlice.LeftRight() + 1,
                      aSlice.TopBottom() + 1);

  
  
  
  
  
  
  if (aRectSize.width < minSize.width) {
    minSize.width = aRectSize.width;
    aSlice.left = 0;
    aSlice.right = 0;
  }
  if (aRectSize.height < minSize.height) {
    minSize.height = aRectSize.height;
    aSlice.top = 0;
    aSlice.bottom = 0;
  }

  MOZ_ASSERT(aSlice.LeftRight() <= minSize.width);
  MOZ_ASSERT(aSlice.TopBottom() <= minSize.height);
  return minSize;
}

void
CacheBlur(DrawTarget& aDT,
          const IntSize& aMinSize,
          const gfxIntSize& aBlurRadius,
          RectCornerRadii* aCornerRadii,
          const gfxRGBA& aShadowColor,
          IntMargin aExtendDest,
          SourceSurface* aBoxShadow)
{
  BlurCacheKey key(aMinSize, aBlurRadius, aCornerRadii, aShadowColor, aDT.GetBackendType());
  BlurCacheData* data = new BlurCacheData(aBoxShadow, aExtendDest, key);
  if (!gBlurCache->RegisterEntry(data)) {
    delete data;
  }
}


static already_AddRefed<SourceSurface>
CreateBlurMask(const IntSize& aRectSize,
               RectCornerRadii* aCornerRadii,
               gfxIntSize aBlurRadius,
               IntMargin& aExtendDestBy,
               IntMargin& aSliceBorder,
               DrawTarget& aDestDrawTarget)
{
  IntMargin slice;
  gfxAlphaBoxBlur blur;
  IntSize minSize =
    ComputeMinSizeForShadowShape(aCornerRadii, aBlurRadius, slice, aRectSize);
  IntRect minRect(IntPoint(), minSize);

  gfxContext* blurCtx = blur.Init(ThebesRect(Rect(minRect)), gfxIntSize(),
                                  aBlurRadius, nullptr, nullptr);

  if (!blurCtx) {
    return nullptr;
  }

  DrawTarget* blurDT = blurCtx->GetDrawTarget();
  ColorPattern black(Color(0.f, 0.f, 0.f, 1.f));

  if (aCornerRadii) {
    RefPtr<Path> roundedRect =
      MakePathForRoundedRect(*blurDT, Rect(minRect), *aCornerRadii);
    blurDT->Fill(roundedRect, black);
  } else {
    blurDT->FillRect(Rect(minRect), black);
  }

  IntPoint topLeft;
  RefPtr<SourceSurface> result = blur.DoBlur(&aDestDrawTarget, &topLeft);

  IntRect expandedMinRect(topLeft, result->GetSize());
  aExtendDestBy = expandedMinRect - minRect;
  aSliceBorder = slice + aExtendDestBy;

  MOZ_ASSERT(aSliceBorder.LeftRight() <= expandedMinRect.width);
  MOZ_ASSERT(aSliceBorder.TopBottom() <= expandedMinRect.height);

  return result.forget();
}

static already_AddRefed<SourceSurface>
CreateBoxShadow(DrawTarget& aDT, SourceSurface* aBlurMask, const gfxRGBA& aShadowColor)
{
  IntSize blurredSize = aBlurMask->GetSize();
  gfxPlatform* platform = gfxPlatform::GetPlatform();
  RefPtr<DrawTarget> boxShadowDT =
    platform->CreateOffscreenContentDrawTarget(blurredSize, SurfaceFormat::B8G8R8A8);

  if (!boxShadowDT) {
    return nullptr;
  }

  if (boxShadowDT->GetType() != aDT.GetType()) {
    printf_stderr("Box shadow type: %hhdi, dest draw target type: %hhdi\n",
      boxShadowDT->GetType(), aDT.GetType());
    MOZ_ASSERT(false, "Box shadows are incorrect type\n");
  }

  ColorPattern shadowColor(ToDeviceColor(aShadowColor));
  boxShadowDT->MaskSurface(shadowColor, aBlurMask, Point(0, 0));
  return boxShadowDT->Snapshot();
}

SourceSurface*
GetBlur(DrawTarget& aDT,
        const IntSize& aRectSize,
        const gfxIntSize& aBlurRadius,
        RectCornerRadii* aCornerRadii,
        const gfxRGBA& aShadowColor,
        IntMargin& aExtendDestBy,
        IntMargin& aSlice)
{
  if (!gBlurCache) {
    gBlurCache = new BlurCache();
  }

  IntSize minSize =
    ComputeMinSizeForShadowShape(aCornerRadii, aBlurRadius, aSlice, aRectSize);

  BlurCacheData* cached = gBlurCache->Lookup(minSize, aBlurRadius,
                                             aCornerRadii, aShadowColor,
                                             aDT.GetBackendType());
  if (cached) {
    
    aExtendDestBy = cached->mExtendDest;
    aSlice = aSlice + aExtendDestBy;
    return cached->mBlur;
  }

  RefPtr<SourceSurface> blurMask =
    CreateBlurMask(aRectSize, aCornerRadii, aBlurRadius, aExtendDestBy, aSlice, aDT);

  if (!blurMask) {
    return nullptr;
  }

  RefPtr<SourceSurface> boxShadow = CreateBoxShadow(aDT, blurMask, aShadowColor);
  if (!boxShadow) {
    return nullptr;
  }

  CacheBlur(aDT, minSize, aBlurRadius, aCornerRadii, aShadowColor, aExtendDestBy, boxShadow);
  return boxShadow;
}

void
gfxAlphaBoxBlur::ShutdownBlurCache()
{
  delete gBlurCache;
  gBlurCache = nullptr;
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
gfxAlphaBoxBlur::BlurRectangle(gfxContext* aDestinationCtx,
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

  RefPtr<SourceSurface> boxShadow = GetBlur(destDrawTarget,
                                            rect.Size(), blurRadius,
                                            aCornerRadii, aShadowColor,
                                            extendDestBy, slice);
  if (!boxShadow) {
    return;
  }

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

