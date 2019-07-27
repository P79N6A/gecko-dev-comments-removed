








#include "SurfaceCache.h"

#include <algorithm>
#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"  
#include "mozilla/DebugOnly.h"
#include "mozilla/Likely.h"
#include "mozilla/Move.h"
#include "mozilla/Mutex.h"
#include "mozilla/RefPtr.h"
#include "mozilla/StaticPtr.h"
#include "nsIMemoryReporter.h"
#include "gfx2DGlue.h"
#include "gfxPattern.h"  
#include "gfxPlatform.h"
#include "gfxPrefs.h"
#include "imgFrame.h"
#include "Image.h"
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

namespace mozilla {

using namespace gfx;

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
  MOZ_DECLARE_REFCOUNTED_TYPENAME(CachedSurface)
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CachedSurface)

  CachedSurface(imgFrame*          aSurface,
                const Cost         aCost,
                const ImageKey     aImageKey,
                const SurfaceKey&  aSurfaceKey,
                const Lifetime     aLifetime)
    : mSurface(aSurface)
    , mCost(aCost)
    , mImageKey(aImageKey)
    , mSurfaceKey(aSurfaceKey)
    , mLifetime(aLifetime)
  {
    MOZ_ASSERT(mSurface, "Must have a valid surface");
    MOZ_ASSERT(mImageKey, "Must have a valid image key");
  }

  DrawableFrameRef DrawableRef() const
  {
    return mSurface->DrawableRef();
  }

  void SetLocked(bool aLocked)
  {
    if (aLocked && mLifetime == Lifetime::Persistent) {
      
      
      
      mDrawableRef = mSurface->DrawableRef();
    } else {
      mDrawableRef.reset();
    }
  }

  bool IsLocked() const { return bool(mDrawableRef); }

  ImageKey GetImageKey() const { return mImageKey; }
  SurfaceKey GetSurfaceKey() const { return mSurfaceKey; }
  CostEntry GetCostEntry() { return image::CostEntry(this, mCost); }
  nsExpirationState* GetExpirationState() { return &mExpirationState; }
  Lifetime GetLifetime() const { return mLifetime; }

  
  struct SizeOfSurfacesSum
  {
    SizeOfSurfacesSum(gfxMemoryLocation aLocation,
                      MallocSizeOf      aMallocSizeOf)
      : mLocation(aLocation)
      , mMallocSizeOf(aMallocSizeOf)
      , mSum(0)
    { }

    void Add(CachedSurface* aCachedSurface)
    {
      MOZ_ASSERT(aCachedSurface, "Should have a CachedSurface");

      if (!aCachedSurface->mSurface) {
        return;
      }
      mSum += aCachedSurface->mSurface->SizeOfExcludingThis(mLocation,
                                                            mMallocSizeOf);
    }

    size_t Result() const { return mSum; }

  private:
    gfxMemoryLocation mLocation;
    MallocSizeOf      mMallocSizeOf;
    size_t            mSum;
  };

private:
  nsExpirationState  mExpirationState;
  nsRefPtr<imgFrame> mSurface;
  DrawableFrameRef   mDrawableRef;
  const Cost         mCost;
  const ImageKey     mImageKey;
  const SurfaceKey   mSurfaceKey;
  const Lifetime     mLifetime;
};










class ImageSurfaceCache
{
  ~ImageSurfaceCache() { }
public:
  ImageSurfaceCache() : mLocked(false) { }

  MOZ_DECLARE_REFCOUNTED_TYPENAME(ImageSurfaceCache)
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ImageSurfaceCache)

  typedef nsRefPtrHashtable<nsGenericHashKey<SurfaceKey>, CachedSurface> SurfaceTable;

  bool IsEmpty() const { return mSurfaces.Count() == 0; }
  
  void Insert(const SurfaceKey& aKey, CachedSurface* aSurface)
  {
    MOZ_ASSERT(aSurface, "Should have a surface");
    MOZ_ASSERT(!mLocked || aSurface->GetLifetime() != Lifetime::Persistent ||
               aSurface->IsLocked(),
               "Inserting an unlocked persistent surface for a locked image");
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

  void SetLocked(bool aLocked) { mLocked = aLocked; }
  bool IsLocked() const { return mLocked; }

private:
  SurfaceTable mSurfaces;
  bool         mLocked;
};








class SurfaceCacheImpl MOZ_FINAL : public nsIMemoryReporter
{
public:
  NS_DECL_ISUPPORTS

  SurfaceCacheImpl(uint32_t aSurfaceCacheExpirationTimeMS,
                   uint32_t aSurfaceCacheDiscardFactor,
                   uint32_t aSurfaceCacheSize)
    : mExpirationTracker(this, aSurfaceCacheExpirationTimeMS)
    , mMemoryPressureObserver(new MemoryPressureObserver)
    , mMutex("SurfaceCache")
    , mDiscardFactor(aSurfaceCacheDiscardFactor)
    , mMaxCost(aSurfaceCacheSize)
    , mAvailableCost(aSurfaceCacheSize)
    , mLockedCost(0)
  {
    nsCOMPtr<nsIObserverService> os = services::GetObserverService();
    if (os)
      os->AddObserver(mMemoryPressureObserver, "memory-pressure", false);
  }

private:
  virtual ~SurfaceCacheImpl()
  {
    nsCOMPtr<nsIObserverService> os = services::GetObserverService();
    if (os)
      os->RemoveObserver(mMemoryPressureObserver, "memory-pressure");

    UnregisterWeakMemoryReporter(this);
  }

public:
  void InitMemoryReporter() { RegisterWeakMemoryReporter(this); }

  Mutex& GetMutex() { return mMutex; }

  InsertOutcome Insert(imgFrame*         aSurface,
                       const Cost        aCost,
                       const ImageKey    aImageKey,
                       const SurfaceKey& aSurfaceKey,
                       Lifetime          aLifetime)
  {
    
    if (MOZ_UNLIKELY(Lookup(aImageKey, aSurfaceKey))) {
      return InsertOutcome::FAILURE_ALREADY_PRESENT;
    }

    
    
    if (MOZ_UNLIKELY(!CanHoldAfterDiscarding(aCost))) {
      return InsertOutcome::FAILURE;
    }

    
    
    while (aCost > mAvailableCost) {
      MOZ_ASSERT(!mCosts.IsEmpty(), "Removed everything and it still won't fit");
      Remove(mCosts.LastElement().GetSurface());
    }

    
    
    nsRefPtr<ImageSurfaceCache> cache = GetImageCache(aImageKey);
    if (!cache) {
      cache = new ImageSurfaceCache;
      mImageCaches.Put(aImageKey, cache);
    }

    nsRefPtr<CachedSurface> surface =
      new CachedSurface(aSurface, aCost, aImageKey, aSurfaceKey, aLifetime);

    
    
    if (cache->IsLocked() && aLifetime == Lifetime::Persistent) {
      surface->SetLocked(true);
      if (!surface->IsLocked()) {
        return InsertOutcome::FAILURE;
      }
    }

    
    MOZ_ASSERT(aCost <= mAvailableCost, "Inserting despite too large a cost");
    cache->Insert(aSurfaceKey, surface);
    StartTracking(surface);

    return InsertOutcome::SUCCESS;
  }

  void Remove(CachedSurface* aSurface)
  {
    MOZ_ASSERT(aSurface, "Should have a surface");
    ImageKey imageKey = aSurface->GetImageKey();

    nsRefPtr<ImageSurfaceCache> cache = GetImageCache(imageKey);
    MOZ_ASSERT(cache, "Shouldn't try to remove a surface with no image cache");

    
    if (aSurface->GetLifetime() == Lifetime::Persistent) {
      static_cast<Image*>(imageKey)->OnSurfaceDiscarded();
    }

    StopTracking(aSurface);
    cache->Remove(aSurface);

    
    
    if (cache->IsEmpty() && !cache->IsLocked()) {
      mImageCaches.Remove(imageKey);
    }
  }

  void StartTracking(CachedSurface* aSurface)
  {
    CostEntry costEntry = aSurface->GetCostEntry();
    MOZ_ASSERT(costEntry.GetCost() <= mAvailableCost,
               "Cost too large and the caller didn't catch it");

    mAvailableCost -= costEntry.GetCost();

    if (aSurface->IsLocked()) {
      mLockedCost += costEntry.GetCost();
      MOZ_ASSERT(mLockedCost <= mMaxCost, "Locked more than we can hold?");
    } else {
      mCosts.InsertElementSorted(costEntry);
      
      
      mExpirationTracker.AddObject(aSurface);
    }
  }

  void StopTracking(CachedSurface* aSurface)
  {
    MOZ_ASSERT(aSurface, "Should have a surface");
    CostEntry costEntry = aSurface->GetCostEntry();

    if (aSurface->IsLocked()) {
      MOZ_ASSERT(mLockedCost >= costEntry.GetCost(), "Costs don't balance");
      mLockedCost -= costEntry.GetCost();
      
      MOZ_ASSERT(!mCosts.Contains(costEntry),
                 "Shouldn't have a cost entry for a locked surface");
    } else {
      if (MOZ_LIKELY(aSurface->GetExpirationState()->IsTracked())) {
        mExpirationTracker.RemoveObject(aSurface);
      } else {
        
        
        NS_WARNING("Not expiration-tracking an unlocked surface!");
      }

      DebugOnly<bool> foundInCosts = mCosts.RemoveElementSorted(costEntry);
      MOZ_ASSERT(foundInCosts, "Lost track of costs for this surface");
    }

    mAvailableCost += costEntry.GetCost();
    MOZ_ASSERT(mAvailableCost <= mMaxCost,
               "More available cost than we started with");
  }

  DrawableFrameRef Lookup(const ImageKey    aImageKey,
                          const SurfaceKey& aSurfaceKey)
  {
    nsRefPtr<ImageSurfaceCache> cache = GetImageCache(aImageKey);
    if (!cache)
      return DrawableFrameRef();  

    nsRefPtr<CachedSurface> surface = cache->Lookup(aSurfaceKey);
    if (!surface)
      return DrawableFrameRef();  

    DrawableFrameRef ref = surface->DrawableRef();
    if (!ref) {
      
      
      Remove(surface);
      return DrawableFrameRef();
    }

    if (!surface->IsLocked()) {
      mExpirationTracker.MarkUsed(surface);
    }

    return ref;
  }

  void RemoveSurface(const ImageKey    aImageKey,
                     const SurfaceKey& aSurfaceKey)
  {
    nsRefPtr<ImageSurfaceCache> cache = GetImageCache(aImageKey);
    if (!cache)
      return;  

    nsRefPtr<CachedSurface> surface = cache->Lookup(aSurfaceKey);
    if (!surface)
      return;  

    Remove(surface);
  }

  bool CanHold(const Cost aCost) const
  {
    return aCost <= mMaxCost;
  }

  void LockImage(const ImageKey aImageKey)
  {
    nsRefPtr<ImageSurfaceCache> cache = GetImageCache(aImageKey);
    if (!cache) {
      cache = new ImageSurfaceCache;
      mImageCaches.Put(aImageKey, cache);
    }

    cache->SetLocked(true);

    
    cache->ForEach(DoLockSurface, this);
  }

  void UnlockImage(const ImageKey aImageKey)
  {
    nsRefPtr<ImageSurfaceCache> cache = GetImageCache(aImageKey);
    if (!cache)
      return;  

    cache->SetLocked(false);

    
    cache->ForEach(DoUnlockSurface, this);
  }

  void RemoveImage(const ImageKey aImageKey)
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

  void DiscardForMemoryPressure()
  {
    
    
    const Cost discardableCost = (mMaxCost - mAvailableCost) - mLockedCost;
    MOZ_ASSERT(discardableCost <= mMaxCost, "Discardable cost doesn't add up");

    
    
    
    
    const Cost targetCost = mAvailableCost + (discardableCost / mDiscardFactor);

    if (targetCost > mMaxCost - mLockedCost) {
      MOZ_ASSERT_UNREACHABLE("Target cost is more than we can discard");
      DiscardAll();
      return;
    }

    
    while (mAvailableCost < targetCost) {
      MOZ_ASSERT(!mCosts.IsEmpty(), "Removed everything and still not done");
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

  static PLDHashOperator DoLockSurface(const SurfaceKey&,
                                       CachedSurface*    aSurface,
                                       void*             aCache)
  {
    if (aSurface->GetLifetime() == Lifetime::Transient ||
        aSurface->IsLocked()) {
      return PL_DHASH_NEXT;
    }

    auto cache = static_cast<SurfaceCacheImpl*>(aCache);
    cache->StopTracking(aSurface);

    
    aSurface->SetLocked(true);
    cache->StartTracking(aSurface);

    return PL_DHASH_NEXT;
  }

  static PLDHashOperator DoUnlockSurface(const SurfaceKey&,
                                         CachedSurface*    aSurface,
                                         void*             aCache)
  {
    if (aSurface->GetLifetime() == Lifetime::Transient ||
        !aSurface->IsLocked()) {
      return PL_DHASH_NEXT;
    }

    auto cache = static_cast<SurfaceCacheImpl*>(aCache);
    cache->StopTracking(aSurface);

    aSurface->SetLocked(false);
    cache->StartTracking(aSurface);

    return PL_DHASH_NEXT;
  }

  NS_IMETHOD
  CollectReports(nsIHandleReportCallback* aHandleReport,
                 nsISupports*             aData,
                 bool                     aAnonymize) MOZ_OVERRIDE
  {
    
    
    
    nsresult rv;

    rv = MOZ_COLLECT_REPORT("imagelib-surface-cache-estimated-total",
                            KIND_OTHER, UNITS_BYTES,
                            (mMaxCost - mAvailableCost),
                            "Estimated total memory used by the imagelib "
                            "surface cache.");
    NS_ENSURE_SUCCESS(rv, rv);

    rv = MOZ_COLLECT_REPORT("imagelib-surface-cache-estimated-locked",
                            KIND_OTHER, UNITS_BYTES,
                            mLockedCost,
                            "Estimated memory used by locked surfaces in the "
                            "imagelib surface cache.");
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  size_t SizeOfSurfaces(const ImageKey    aImageKey,
                        gfxMemoryLocation aLocation,
                        MallocSizeOf      aMallocSizeOf)
  {
    nsRefPtr<ImageSurfaceCache> cache = GetImageCache(aImageKey);
    if (!cache) {
      return 0;  
    }

    
    CachedSurface::SizeOfSurfacesSum sum(aLocation, aMallocSizeOf);
    cache->ForEach(DoSizeOfSurfacesSum, &sum);

    return sum.Result();
  }

  static PLDHashOperator DoSizeOfSurfacesSum(const SurfaceKey&,
                                             CachedSurface*    aSurface,
                                             void*             aSum)
  {
    auto sum = static_cast<CachedSurface::SizeOfSurfacesSum*>(aSum);
    sum->Add(aSurface);
    return PL_DHASH_NEXT;
  }

private:
  already_AddRefed<ImageSurfaceCache> GetImageCache(const ImageKey aImageKey)
  {
    nsRefPtr<ImageSurfaceCache> imageCache;
    mImageCaches.Get(aImageKey, getter_AddRefs(imageCache));
    return imageCache.forget();
  }

  
  
  
  
  
  bool CanHoldAfterDiscarding(const Cost aCost) const
  {
    return aCost <= mMaxCost - mLockedCost;
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

    NS_IMETHOD Observe(nsISupports*,
                       const char* aTopic,
                       const char16_t*) MOZ_OVERRIDE
    {
      if (sInstance && strcmp(aTopic, "memory-pressure") == 0) {
        sInstance->DiscardForMemoryPressure();
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
  Mutex                                                     mMutex;
  const uint32_t                                            mDiscardFactor;
  const Cost                                                mMaxCost;
  Cost                                                      mAvailableCost;
  Cost                                                      mLockedCost;
};

NS_IMPL_ISUPPORTS(SurfaceCacheImpl, nsIMemoryReporter)
NS_IMPL_ISUPPORTS(SurfaceCacheImpl::MemoryPressureObserver, nsIObserver)





 void
SurfaceCache::Initialize()
{
  
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!sInstance, "Shouldn't initialize more than once");

  

  
  
  uint32_t surfaceCacheExpirationTimeMS =
    gfxPrefs::ImageMemSurfaceCacheMinExpirationMS();

  
  
  
  
  uint32_t surfaceCacheDiscardFactor =
    max(gfxPrefs::ImageMemSurfaceCacheDiscardFactor(), 1u);

  
  uint64_t surfaceCacheMaxSizeKB = gfxPrefs::ImageMemSurfaceCacheMaxSizeKB();

  
  
  
  
  
  
  
  
  uint32_t surfaceCacheSizeFactor =
    max(gfxPrefs::ImageMemSurfaceCacheSizeFactor(), 1u);

  
  uint64_t memorySize = PR_GetPhysicalMemorySize();
  if (memorySize == 0) {
    MOZ_ASSERT_UNREACHABLE("PR_GetPhysicalMemorySize not implemented here");
    memorySize = 256 * 1024 * 1024;  
  }
  uint64_t proposedSize = memorySize / surfaceCacheSizeFactor;
  uint64_t surfaceCacheSizeBytes = min(proposedSize, surfaceCacheMaxSizeKB * 1024);
  uint32_t finalSurfaceCacheSizeBytes =
    min(surfaceCacheSizeBytes, uint64_t(UINT32_MAX));

  
  
  
  sInstance = new SurfaceCacheImpl(surfaceCacheExpirationTimeMS,
                                   surfaceCacheDiscardFactor,
                                   finalSurfaceCacheSizeBytes);
  sInstance->InitMemoryReporter();
}

 void
SurfaceCache::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(sInstance, "No singleton - was Shutdown() called twice?");
  sInstance = nullptr;
}

 DrawableFrameRef
SurfaceCache::Lookup(const ImageKey    aImageKey,
                     const SurfaceKey& aSurfaceKey)
{
  if (!sInstance) {
    return DrawableFrameRef();
  }

  MutexAutoLock lock(sInstance->GetMutex());
  return sInstance->Lookup(aImageKey, aSurfaceKey);
}

 InsertOutcome
SurfaceCache::Insert(imgFrame*         aSurface,
                     const ImageKey    aImageKey,
                     const SurfaceKey& aSurfaceKey,
                     Lifetime          aLifetime)
{
  if (!sInstance) {
    return InsertOutcome::FAILURE;
  }

  MutexAutoLock lock(sInstance->GetMutex());
  Cost cost = ComputeCost(aSurfaceKey.Size());
  return sInstance->Insert(aSurface, cost, aImageKey, aSurfaceKey, aLifetime);
}

 bool
SurfaceCache::CanHold(const IntSize& aSize)
{
  if (!sInstance) {
    return false;
  }

  Cost cost = ComputeCost(aSize);
  return sInstance->CanHold(cost);
}

 bool
SurfaceCache::CanHold(size_t aSize)
{
  if (!sInstance) {
    return false;
  }

  return sInstance->CanHold(aSize);
}

 void
SurfaceCache::LockImage(Image* aImageKey)
{
  if (sInstance) {
    MutexAutoLock lock(sInstance->GetMutex());
    return sInstance->LockImage(aImageKey);
  }
}

 void
SurfaceCache::UnlockImage(Image* aImageKey)
{
  if (sInstance) {
    MutexAutoLock lock(sInstance->GetMutex());
    return sInstance->UnlockImage(aImageKey);
  }
}

 void
SurfaceCache::RemoveSurface(const ImageKey    aImageKey,
                            const SurfaceKey& aSurfaceKey)
{
  if (sInstance) {
    MutexAutoLock lock(sInstance->GetMutex());
    sInstance->RemoveSurface(aImageKey, aSurfaceKey);
  }
}

 void
SurfaceCache::RemoveImage(Image* aImageKey)
{
  if (sInstance) {
    MutexAutoLock lock(sInstance->GetMutex());
    sInstance->RemoveImage(aImageKey);
  }
}

 void
SurfaceCache::DiscardAll()
{
  if (sInstance) {
    MutexAutoLock lock(sInstance->GetMutex());
    sInstance->DiscardAll();
  }
}

 size_t
SurfaceCache::SizeOfSurfaces(const ImageKey    aImageKey,
                             gfxMemoryLocation aLocation,
                             MallocSizeOf      aMallocSizeOf)
{
  if (!sInstance) {
    return 0;
  }

  MutexAutoLock lock(sInstance->GetMutex());
  return sInstance->SizeOfSurfaces(aImageKey, aLocation, aMallocSizeOf);
}

} 
} 
