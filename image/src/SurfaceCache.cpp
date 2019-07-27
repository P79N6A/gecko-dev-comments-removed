








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

static Cost
ComputeCost(const IntSize& aSize)
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
  ~CachedSurface() { }
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
  bool IsDecoded() const { return mSurface->IsImageComplete(); }

  
  struct MOZ_STACK_CLASS SurfaceMemoryReport
  {
    SurfaceMemoryReport(nsTArray<SurfaceMemoryCounter>& aCounters,
                        MallocSizeOf                    aMallocSizeOf)
      : mCounters(aCounters)
      , mMallocSizeOf(aMallocSizeOf)
    { }

    void Add(CachedSurface* aCachedSurface)
    {
      MOZ_ASSERT(aCachedSurface, "Should have a CachedSurface");

      SurfaceMemoryCounter counter(aCachedSurface->GetSurfaceKey(),
                                   aCachedSurface->IsLocked());

      if (aCachedSurface->mSurface) {
        counter.SubframeSize() = Some(aCachedSurface->mSurface->GetSize());

        size_t heap = aCachedSurface->mSurface
          ->SizeOfExcludingThis(gfxMemoryLocation::IN_PROCESS_HEAP,
                                mMallocSizeOf);
        counter.Values().SetDecodedHeap(heap);

        size_t nonHeap = aCachedSurface->mSurface
          ->SizeOfExcludingThis(gfxMemoryLocation::IN_PROCESS_NONHEAP, nullptr);
        counter.Values().SetDecodedNonHeap(nonHeap);
      }

      mCounters.AppendElement(counter);
    }

  private:
    nsTArray<SurfaceMemoryCounter>& mCounters;
    MallocSizeOf                    mMallocSizeOf;
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

  typedef
    nsRefPtrHashtable<nsGenericHashKey<SurfaceKey>, CachedSurface> SurfaceTable;

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

  already_AddRefed<CachedSurface>
  LookupBestMatch(const SurfaceKey&      aSurfaceKey,
                  const Maybe<uint32_t>& aAlternateFlags)
  {
    
    nsRefPtr<CachedSurface> surface;
    mSurfaces.Get(aSurfaceKey, getter_AddRefs(surface));
    if (surface && surface->IsDecoded()) {
      return surface.forget();
    }

    
    MatchContext matchContext(aSurfaceKey, aAlternateFlags);
    ForEach(TryToImproveMatch, &matchContext);
    return matchContext.mBestMatch.forget();
  }

  void ForEach(SurfaceTable::EnumReadFunction aFunction, void* aData)
  {
    mSurfaces.EnumerateRead(aFunction, aData);
  }

  void SetLocked(bool aLocked) { mLocked = aLocked; }
  bool IsLocked() const { return mLocked; }

private:
  struct MatchContext
  {
    MatchContext(const SurfaceKey& aIdealKey,
                 const Maybe<uint32_t>& aAlternateFlags)
      : mIdealKey(aIdealKey)
      , mAlternateFlags(aAlternateFlags)
    { }

    const SurfaceKey& mIdealKey;
    const Maybe<uint32_t> mAlternateFlags;
    nsRefPtr<CachedSurface> mBestMatch;
  };

  static PLDHashOperator TryToImproveMatch(const SurfaceKey& aSurfaceKey,
                                           CachedSurface*    aSurface,
                                           void*             aContext)
  {
    auto context = static_cast<MatchContext*>(aContext);
    const SurfaceKey& idealKey = context->mIdealKey;

    
    if (aSurfaceKey.AnimationTime() != idealKey.AnimationTime() ||
        aSurfaceKey.SVGContext() != idealKey.SVGContext()) {
      return PL_DHASH_NEXT;
    }

    
    
    if (aSurfaceKey.Flags() != idealKey.Flags() &&
        Some(aSurfaceKey.Flags()) != context->mAlternateFlags) {
      return PL_DHASH_NEXT;
    }

    
    
    if (!context->mBestMatch) {
      context->mBestMatch = aSurface;
      return PL_DHASH_NEXT;
    }

    MOZ_ASSERT(context->mBestMatch, "Should have a current best match");

    
    bool bestMatchIsDecoded = context->mBestMatch->IsDecoded();
    if (bestMatchIsDecoded && !aSurface->IsDecoded()) {
      return PL_DHASH_NEXT;
    }
    if (!bestMatchIsDecoded && aSurface->IsDecoded()) {
      context->mBestMatch = aSurface;
      return PL_DHASH_NEXT;
    }

    SurfaceKey bestMatchKey = context->mBestMatch->GetSurfaceKey();

    
    
    
    int64_t idealArea = idealKey.Size().width * idealKey.Size().height;
    int64_t surfaceArea = aSurfaceKey.Size().width * aSurfaceKey.Size().height;
    int64_t bestMatchArea =
      bestMatchKey.Size().width * bestMatchKey.Size().height;

    
    if (bestMatchArea < idealArea) {
      if (surfaceArea > bestMatchArea) {
        context->mBestMatch = aSurface;
      }
      return PL_DHASH_NEXT;
    }

    
    if (idealArea <= surfaceArea && surfaceArea < bestMatchArea) {
      context->mBestMatch = aSurface;
      return PL_DHASH_NEXT;
    }

    
    return PL_DHASH_NEXT;
  }

  SurfaceTable mSurfaces;
  bool         mLocked;
};








class SurfaceCacheImpl final : public nsIMemoryReporter
{
public:
  NS_DECL_ISUPPORTS

  SurfaceCacheImpl(uint32_t aSurfaceCacheExpirationTimeMS,
                   uint32_t aSurfaceCacheDiscardFactor,
                   uint32_t aSurfaceCacheSize)
    : mExpirationTracker(aSurfaceCacheExpirationTimeMS)
    , mMemoryPressureObserver(new MemoryPressureObserver)
    , mMutex("SurfaceCache")
    , mDiscardFactor(aSurfaceCacheDiscardFactor)
    , mMaxCost(aSurfaceCacheSize)
    , mAvailableCost(aSurfaceCacheSize)
    , mLockedCost(0)
  {
    nsCOMPtr<nsIObserverService> os = services::GetObserverService();
    if (os) {
      os->AddObserver(mMemoryPressureObserver, "memory-pressure", false);
    }
  }

private:
  virtual ~SurfaceCacheImpl()
  {
    nsCOMPtr<nsIObserverService> os = services::GetObserverService();
    if (os) {
      os->RemoveObserver(mMemoryPressureObserver, "memory-pressure");
    }

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
      MOZ_ASSERT(!mCosts.IsEmpty(),
                 "Removed everything and it still won't fit");
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
    if (!cache) {
      return DrawableFrameRef();  
    }

    nsRefPtr<CachedSurface> surface = cache->Lookup(aSurfaceKey);
    if (!surface) {
      return DrawableFrameRef();  
    }

    DrawableFrameRef ref = surface->DrawableRef();
    if (!ref) {
      
      
      Remove(surface);
      return DrawableFrameRef();
    }

    if (cache->IsLocked()) {
      LockSurface(surface);
    } else {
      mExpirationTracker.MarkUsed(surface);
    }

    return ref;
  }

  DrawableFrameRef LookupBestMatch(const ImageKey         aImageKey,
                                   const SurfaceKey&      aSurfaceKey,
                                   const Maybe<uint32_t>& aAlternateFlags)
  {
    nsRefPtr<ImageSurfaceCache> cache = GetImageCache(aImageKey);
    if (!cache) {
      return DrawableFrameRef();  
    }

    
    
    
    
    

    nsRefPtr<CachedSurface> surface;
    DrawableFrameRef ref;
    while (true) {
      surface = cache->LookupBestMatch(aSurfaceKey, aAlternateFlags);
      if (!surface) {
        return DrawableFrameRef();  
      }

      ref = surface->DrawableRef();
      if (ref) {
        break;
      }

      
      
      Remove(surface);
    }

    if (cache->IsLocked()) {
      LockSurface(surface);
    } else {
      mExpirationTracker.MarkUsed(surface);
    }

    return ref;
  }

  void RemoveSurface(const ImageKey    aImageKey,
                     const SurfaceKey& aSurfaceKey)
  {
    nsRefPtr<ImageSurfaceCache> cache = GetImageCache(aImageKey);
    if (!cache) {
      return;  
    }

    nsRefPtr<CachedSurface> surface = cache->Lookup(aSurfaceKey);
    if (!surface) {
      return;  
    }

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

    
    
  }

  void UnlockImage(const ImageKey aImageKey)
  {
    nsRefPtr<ImageSurfaceCache> cache = GetImageCache(aImageKey);
    if (!cache || !cache->IsLocked()) {
      return;  
    }

    cache->SetLocked(false);

    
    cache->ForEach(DoUnlockSurface, this);
  }

  void UnlockSurfaces(const ImageKey aImageKey)
  {
    nsRefPtr<ImageSurfaceCache> cache = GetImageCache(aImageKey);
    if (!cache || !cache->IsLocked()) {
      return;  
    }

    
    

    
    cache->ForEach(DoUnlockSurface, this);
  }

  void RemoveImage(const ImageKey aImageKey)
  {
    nsRefPtr<ImageSurfaceCache> cache = GetImageCache(aImageKey);
    if (!cache) {
      return;  
    }

    
    
    
    
    
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

  void LockSurface(CachedSurface* aSurface)
  {
    if (aSurface->GetLifetime() == Lifetime::Transient ||
        aSurface->IsLocked()) {
      return;
    }

    StopTracking(aSurface);

    
    aSurface->SetLocked(true);
    StartTracking(aSurface);
  }

  static PLDHashOperator DoStopTracking(const SurfaceKey&,
                                        CachedSurface*    aSurface,
                                        void*             aCache)
  {
    static_cast<SurfaceCacheImpl*>(aCache)->StopTracking(aSurface);
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
                 bool                     aAnonymize) override
  {
    MutexAutoLock lock(mMutex);

    
    
    
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

  void CollectSizeOfSurfaces(const ImageKey                  aImageKey,
                             nsTArray<SurfaceMemoryCounter>& aCounters,
                             MallocSizeOf                    aMallocSizeOf)
  {
    nsRefPtr<ImageSurfaceCache> cache = GetImageCache(aImageKey);
    if (!cache) {
      return;  
    }

    
    CachedSurface::SurfaceMemoryReport report(aCounters, aMallocSizeOf);
    cache->ForEach(DoCollectSizeOfSurface, &report);
  }

  static PLDHashOperator DoCollectSizeOfSurface(const SurfaceKey&,
                                                CachedSurface*    aSurface,
                                                void*             aReport)
  {
    auto report = static_cast<CachedSurface::SurfaceMemoryReport*>(aReport);
    report->Add(aSurface);
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
    explicit SurfaceTracker(uint32_t aSurfaceCacheExpirationTimeMS)
      : nsExpirationTracker<CachedSurface, 2>(aSurfaceCacheExpirationTimeMS)
    { }

  protected:
    virtual void NotifyExpired(CachedSurface* aSurface) override
    {
      if (sInstance) {
        MutexAutoLock lock(sInstance->GetMutex());
        sInstance->Remove(aSurface);
      }
    }
  };

  struct MemoryPressureObserver : public nsIObserver
  {
    NS_DECL_ISUPPORTS

    NS_IMETHOD Observe(nsISupports*,
                       const char* aTopic,
                       const char16_t*) override
    {
      if (sInstance && strcmp(aTopic, "memory-pressure") == 0) {
        MutexAutoLock lock(sInstance->GetMutex());
        sInstance->DiscardForMemoryPressure();
      }
      return NS_OK;
    }

  private:
    virtual ~MemoryPressureObserver() { }
  };

  nsTArray<CostEntry>                     mCosts;
  nsRefPtrHashtable<nsPtrHashKey<Image>,
    ImageSurfaceCache> mImageCaches;
  SurfaceTracker                          mExpirationTracker;
  nsRefPtr<MemoryPressureObserver>        mMemoryPressureObserver;
  Mutex                                   mMutex;
  const uint32_t                          mDiscardFactor;
  const Cost                              mMaxCost;
  Cost                                    mAvailableCost;
  Cost                                    mLockedCost;
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
  uint64_t surfaceCacheSizeBytes = min(proposedSize,
                                       surfaceCacheMaxSizeKB * 1024);
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
SurfaceCache::Lookup(const ImageKey         aImageKey,
                     const SurfaceKey&      aSurfaceKey,
                     const Maybe<uint32_t>& aAlternateFlags )
{
  if (!sInstance) {
    return DrawableFrameRef();
  }

  MutexAutoLock lock(sInstance->GetMutex());

  DrawableFrameRef ref = sInstance->Lookup(aImageKey, aSurfaceKey);
  if (!ref && aAlternateFlags) {
    ref = sInstance->Lookup(aImageKey,
                            aSurfaceKey.WithNewFlags(*aAlternateFlags));
  }

  return ref;
}

 DrawableFrameRef
SurfaceCache::LookupBestMatch(const ImageKey         aImageKey,
                              const SurfaceKey&      aSurfaceKey,
                              const Maybe<uint32_t>& aAlternateFlags
                                )
{
  if (!sInstance) {
    return DrawableFrameRef();
  }

  MutexAutoLock lock(sInstance->GetMutex());
  return sInstance->LookupBestMatch(aImageKey, aSurfaceKey, aAlternateFlags);
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
SurfaceCache::UnlockSurfaces(const ImageKey aImageKey)
{
  if (sInstance) {
    MutexAutoLock lock(sInstance->GetMutex());
    return sInstance->UnlockSurfaces(aImageKey);
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

 void
SurfaceCache::CollectSizeOfSurfaces(const ImageKey                  aImageKey,
                                    nsTArray<SurfaceMemoryCounter>& aCounters,
                                    MallocSizeOf                    aMallocSizeOf)
{
  if (!sInstance) {
    return;
  }

  MutexAutoLock lock(sInstance->GetMutex());
  return sInstance->CollectSizeOfSurfaces(aImageKey, aCounters, aMallocSizeOf);
}

} 
} 
