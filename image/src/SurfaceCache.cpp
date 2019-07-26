








#include "SurfaceCache.h"

#include <algorithm>
#include "mozilla/Attributes.h"  
#include "mozilla/DebugOnly.h"
#include "mozilla/Preferences.h"
#include "mozilla/RefPtr.h"
#include "mozilla/StaticPtr.h"
#include "nsIMemoryReporter.h"
#include "gfx2DGlue.h"
#include "gfxPattern.h"  
#include "gfxDrawable.h"
#include "gfxPlatform.h"
#include "nsAutoPtr.h"
#include "nsExpirationTracker.h"
#include "nsHashKeys.h"
#include "nsRefPtrHashtable.h"
#include "nsSize.h"
#include "nsTArray.h"
#include "prsystem.h"
#include "SVGImageContext.h"

using std::max;
using std::min;
using namespace mozilla::gfx;

namespace mozilla {
namespace image {

class CachedSurface;
class SurfaceCacheImpl;






static StaticRefPtr<SurfaceCacheImpl> sInstance;












typedef size_t Cost;

static Cost ComputeCost(const IntSize& aSize)
{
  return aSize.width * aSize.height * 4;  
}












class CostEntry
{
public:
  CostEntry(CachedSurface* aSurface, Cost aCost)
    : mSurface(aSurface)
    , mCost(aCost)
  {
    MOZ_ASSERT(aSurface, "Must have a surface");
  }

  CachedSurface* GetSurface() const { return mSurface; }
  Cost GetCost() const { return mCost; }

  bool operator==(const CostEntry& aOther) const
  {
    return mSurface == aOther.mSurface &&
           mCost == aOther.mCost;
  }

  bool operator<(const CostEntry& aOther) const
  {
    return mCost < aOther.mCost ||
           (mCost == aOther.mCost && mSurface < aOther.mSurface);
  }

private:
  CachedSurface* mSurface;
  Cost           mCost;
};





class CachedSurface
{
  ~CachedSurface() {}
public:
  NS_INLINE_DECL_REFCOUNTING(CachedSurface)

  CachedSurface(DrawTarget*       aTarget,
                const IntSize     aTargetSize,
                const Cost        aCost,
                const ImageKey    aImageKey,
                const SurfaceKey& aSurfaceKey)
    : mTarget(aTarget)
    , mTargetSize(aTargetSize)
    , mCost(aCost)
    , mImageKey(aImageKey)
    , mSurfaceKey(aSurfaceKey)
  {
    MOZ_ASSERT(mTarget, "Must have a valid DrawTarget");
    MOZ_ASSERT(mImageKey, "Must have a valid image key");
  }

  already_AddRefed<gfxDrawable> Drawable() const
  {
    nsRefPtr<gfxDrawable> drawable =
      new gfxSurfaceDrawable(mTarget, ThebesIntSize(mTargetSize));
    return drawable.forget();
  }

  ImageKey GetImageKey() const { return mImageKey; }
  SurfaceKey GetSurfaceKey() const { return mSurfaceKey; }
  CostEntry GetCostEntry() { return image::CostEntry(this, mCost); }
  nsExpirationState* GetExpirationState() { return &mExpirationState; }

private:
  nsExpirationState       mExpirationState;
  nsRefPtr<DrawTarget>    mTarget;
  const IntSize           mTargetSize;
  const Cost              mCost;
  const ImageKey          mImageKey;
  const SurfaceKey        mSurfaceKey;
};







class ImageSurfaceCache
{
  ~ImageSurfaceCache() {}
public:
  NS_INLINE_DECL_REFCOUNTING(ImageSurfaceCache)

  typedef nsRefPtrHashtable<nsGenericHashKey<SurfaceKey>, CachedSurface> SurfaceTable;

  bool IsEmpty() const { return mSurfaces.Count() == 0; }
  
  void Insert(const SurfaceKey& aKey, CachedSurface* aSurface)
  {
    MOZ_ASSERT(aSurface, "Should have a surface");
    mSurfaces.Put(aKey, aSurface);
  }

  void Remove(CachedSurface* aSurface)
  {
    MOZ_ASSERT(aSurface, "Should have a surface");
    MOZ_ASSERT(mSurfaces.GetWeak(aSurface->GetSurfaceKey()),
        "Should not be removing a surface we don't have");

    mSurfaces.Remove(aSurface->GetSurfaceKey());
  }

  already_AddRefed<CachedSurface> Lookup(const SurfaceKey& aSurfaceKey)
  {
    nsRefPtr<CachedSurface> surface;
    mSurfaces.Get(aSurfaceKey, getter_AddRefs(surface));
    return surface.forget();
  }

  void ForEach(SurfaceTable::EnumReadFunction aFunction, void* aData)
  {
    mSurfaces.EnumerateRead(aFunction, aData);
  }

private:
  SurfaceTable mSurfaces;
};








class SurfaceCacheImpl : public nsIMemoryReporter
{
public:
  NS_DECL_ISUPPORTS

  SurfaceCacheImpl(uint32_t aSurfaceCacheExpirationTimeMS,
                   uint32_t aSurfaceCacheSize)
    : mExpirationTracker(MOZ_THIS_IN_INITIALIZER_LIST(),
                         aSurfaceCacheExpirationTimeMS)
    , mMemoryPressureObserver(new MemoryPressureObserver)
    , mMaxCost(aSurfaceCacheSize)
    , mAvailableCost(aSurfaceCacheSize)
  {
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (os)
      os->AddObserver(mMemoryPressureObserver, "memory-pressure", false);
  }

private:
  virtual ~SurfaceCacheImpl()
  {
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (os)
      os->RemoveObserver(mMemoryPressureObserver, "memory-pressure");

    UnregisterWeakMemoryReporter(this);
  }

public:
  void InitMemoryReporter() {
    RegisterWeakMemoryReporter(this);
  }

  void Insert(DrawTarget*       aTarget,
              IntSize           aTargetSize,
              const Cost        aCost,
              const ImageKey    aImageKey,
              const SurfaceKey& aSurfaceKey)
  {
    MOZ_ASSERT(!Lookup(aImageKey, aSurfaceKey).take(),
               "Inserting a duplicate drawable into the SurfaceCache");

    
    if (!CanHold(aCost))
      return;

    nsRefPtr<CachedSurface> surface =
      new CachedSurface(aTarget, aTargetSize, aCost, aImageKey, aSurfaceKey);

    
    while (aCost > mAvailableCost) {
      MOZ_ASSERT(!mCosts.IsEmpty(), "Removed everything and it still won't fit");
      Remove(mCosts.LastElement().GetSurface());
    }

    
    
    nsRefPtr<ImageSurfaceCache> cache = GetImageCache(aImageKey);
    if (!cache) {
      cache = new ImageSurfaceCache;
      mImageCaches.Put(aImageKey, cache);
    }

    
    MOZ_ASSERT(aCost <= mAvailableCost, "Inserting despite too large a cost");
    cache->Insert(aSurfaceKey, surface);
    StartTracking(surface);
  }

  void Remove(CachedSurface* aSurface)
  {
    MOZ_ASSERT(aSurface, "Should have a surface");
    const ImageKey imageKey = aSurface->GetImageKey();

    nsRefPtr<ImageSurfaceCache> cache = GetImageCache(imageKey);
    MOZ_ASSERT(cache, "Shouldn't try to remove a surface with no image cache");

    StopTracking(aSurface);
    cache->Remove(aSurface);

    
    if (cache->IsEmpty()) {
      mImageCaches.Remove(imageKey);
    }
  }

  void StartTracking(CachedSurface* aSurface)
  {
    CostEntry costEntry = aSurface->GetCostEntry();
    MOZ_ASSERT(costEntry.GetCost() <= mAvailableCost,
               "Cost too large and the caller didn't catch it");

    mAvailableCost -= costEntry.GetCost();
    mCosts.InsertElementSorted(costEntry);
    mExpirationTracker.AddObject(aSurface);
  }

  void StopTracking(CachedSurface* aSurface)
  {
    MOZ_ASSERT(aSurface, "Should have a surface");
    CostEntry costEntry = aSurface->GetCostEntry();

    mExpirationTracker.RemoveObject(aSurface);
    DebugOnly<bool> foundInCosts = mCosts.RemoveElementSorted(costEntry);
    mAvailableCost += costEntry.GetCost();

    MOZ_ASSERT(foundInCosts, "Lost track of costs for this surface");
    MOZ_ASSERT(mAvailableCost <= mMaxCost, "More available cost than we started with");
  }

  already_AddRefed<gfxDrawable> Lookup(const ImageKey    aImageKey,
                                       const SurfaceKey& aSurfaceKey)
  {
    nsRefPtr<ImageSurfaceCache> cache = GetImageCache(aImageKey);
    if (!cache)
      return nullptr;  
    
    nsRefPtr<CachedSurface> surface = cache->Lookup(aSurfaceKey);
    if (!surface)
      return nullptr;  
    
    mExpirationTracker.MarkUsed(surface);
    return surface->Drawable();
  }

  bool CanHold(const Cost aCost) const
  {
    return aCost <= mMaxCost;
  }

  void Discard(const ImageKey aImageKey)
  {
    nsRefPtr<ImageSurfaceCache> cache = GetImageCache(aImageKey);
    if (!cache)
      return;  

    
    
    
    
    
    cache->ForEach(DoStopTracking, this);

    
    mImageCaches.Remove(aImageKey);
  }

  void DiscardAll()
  {
    
    
    while (!mCosts.IsEmpty()) {
      Remove(mCosts.LastElement().GetSurface());
    }
  }

  static PLDHashOperator DoStopTracking(const SurfaceKey&,
                                        CachedSurface*    aSurface,
                                        void*             aCache)
  {
    static_cast<SurfaceCacheImpl*>(aCache)->StopTracking(aSurface);
    return PL_DHASH_NEXT;
  }

  NS_IMETHOD
  CollectReports(nsIHandleReportCallback* aHandleReport, nsISupports* aData,
                 bool aAnonymize)
  {
    return MOZ_COLLECT_REPORT(
      "imagelib-surface-cache", KIND_OTHER, UNITS_BYTES,
      SizeOfSurfacesEstimate(),
      "Memory used by the imagelib temporary surface cache.");
  }

  
  
  
  
  Cost SizeOfSurfacesEstimate() const
  {
    return mMaxCost - mAvailableCost;
  }

private:
  already_AddRefed<ImageSurfaceCache> GetImageCache(const ImageKey aImageKey)
  {
    nsRefPtr<ImageSurfaceCache> imageCache;
    mImageCaches.Get(aImageKey, getter_AddRefs(imageCache));
    return imageCache.forget();
  }

  struct SurfaceTracker : public nsExpirationTracker<CachedSurface, 2>
  {
    SurfaceTracker(SurfaceCacheImpl* aCache, uint32_t aSurfaceCacheExpirationTimeMS)
      : nsExpirationTracker<CachedSurface, 2>(aSurfaceCacheExpirationTimeMS)
      , mCache(aCache)
    { }

  protected:
    virtual void NotifyExpired(CachedSurface* aSurface) MOZ_OVERRIDE
    {
      if (mCache) {
        mCache->Remove(aSurface);
      }
    }

  private:
    SurfaceCacheImpl* const mCache;  
  };

  struct MemoryPressureObserver : public nsIObserver
  {
    NS_DECL_ISUPPORTS

    NS_IMETHOD Observe(nsISupports*, const char* aTopic, const char16_t*)
    {
      if (sInstance && strcmp(aTopic, "memory-pressure") == 0) {
        sInstance->DiscardAll();
      }
      return NS_OK;
    }

  private:
    virtual ~MemoryPressureObserver() { }
  };


  nsTArray<CostEntry>                                       mCosts;
  nsRefPtrHashtable<nsPtrHashKey<Image>, ImageSurfaceCache> mImageCaches;
  SurfaceTracker                                            mExpirationTracker;
  nsRefPtr<MemoryPressureObserver>                          mMemoryPressureObserver;
  const Cost                                                mMaxCost;
  Cost                                                      mAvailableCost;
};

NS_IMPL_ISUPPORTS(SurfaceCacheImpl, nsIMemoryReporter)
NS_IMPL_ISUPPORTS(SurfaceCacheImpl::MemoryPressureObserver, nsIObserver)





 void
SurfaceCache::Initialize()
{
  
  MOZ_ASSERT(!sInstance, "Shouldn't initialize more than once");

  
  
  uint32_t surfaceCacheExpirationTimeMS =
    Preferences::GetUint("image.mem.surfacecache.min_expiration_ms", 60 * 1000);

  
  
  uint32_t surfaceCacheMaxSizeKB =
    Preferences::GetUint("image.mem.surfacecache.max_size_kb", 100 * 1024);

  
  
  
  
  
  
  
  uint32_t surfaceCacheSizeFactor =
    Preferences::GetUint("image.mem.surfacecache.size_factor", 64);

  
  surfaceCacheSizeFactor = max(surfaceCacheSizeFactor, 1u);

  
  uint32_t proposedSize = PR_GetPhysicalMemorySize() / surfaceCacheSizeFactor;
  uint32_t surfaceCacheSizeBytes = min(proposedSize, surfaceCacheMaxSizeKB * 1024);

  
  
  
  sInstance = new SurfaceCacheImpl(surfaceCacheExpirationTimeMS,
                                   surfaceCacheSizeBytes);
  sInstance->InitMemoryReporter();
}

 void
SurfaceCache::Shutdown()
{
  MOZ_ASSERT(sInstance, "No singleton - was Shutdown() called twice?");
  sInstance = nullptr;
}

 already_AddRefed<gfxDrawable>
SurfaceCache::Lookup(const ImageKey    aImageKey,
                     const SurfaceKey& aSurfaceKey)
{
  MOZ_ASSERT(sInstance, "Should be initialized");
  MOZ_ASSERT(NS_IsMainThread());

  return sInstance->Lookup(aImageKey, aSurfaceKey);
}

 void
SurfaceCache::Insert(DrawTarget*       aTarget,
                     const ImageKey    aImageKey,
                     const SurfaceKey& aSurfaceKey)
{
  MOZ_ASSERT(sInstance, "Should be initialized");
  MOZ_ASSERT(NS_IsMainThread());

  Cost cost = ComputeCost(aSurfaceKey.Size());
  return sInstance->Insert(aTarget, aSurfaceKey.Size(), cost, aImageKey,
                           aSurfaceKey);
}

 bool
SurfaceCache::CanHold(const IntSize& aSize)
{
  MOZ_ASSERT(sInstance, "Should be initialized");
  MOZ_ASSERT(NS_IsMainThread());

  Cost cost = ComputeCost(aSize);
  return sInstance->CanHold(cost);
}

 void
SurfaceCache::Discard(Image* aImageKey)
{
  MOZ_ASSERT(sInstance, "Should be initialized");
  MOZ_ASSERT(NS_IsMainThread());

  return sInstance->Discard(aImageKey);
}

 void
SurfaceCache::DiscardAll()
{
  MOZ_ASSERT(sInstance, "Should be initialized");
  MOZ_ASSERT(NS_IsMainThread());

  return sInstance->DiscardAll();
}

} 
} 
