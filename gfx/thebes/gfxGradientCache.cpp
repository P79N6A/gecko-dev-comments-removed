




#include "mozilla/gfx/2D.h"
#include "nsTArray.h"
#include "pldhash.h"
#include "nsExpirationTracker.h"
#include "nsClassHashtable.h"
#include "mozilla/Telemetry.h"
#include "gfxGradientCache.h"
#include <time.h>

namespace mozilla {
namespace gfx {

using namespace mozilla;

struct GradientCacheKey : public PLDHashEntryHdr {
  typedef const GradientCacheKey& KeyType;
  typedef const GradientCacheKey* KeyTypePointer;
  enum { ALLOW_MEMMOVE = true };
  const nsTArray<GradientStop> mStops;
  ExtendMode mExtend;
  BackendType mBackendType;

  GradientCacheKey(const nsTArray<GradientStop>& aStops, ExtendMode aExtend, BackendType aBackendType)
    : mStops(aStops), mExtend(aExtend), mBackendType(aBackendType)
  { }

  explicit GradientCacheKey(const GradientCacheKey* aOther)
    : mStops(aOther->mStops), mExtend(aOther->mExtend), mBackendType(aOther->mBackendType)
  { }

  union FloatUint32
  {
    float    f;
    uint32_t u;
  };

  static PLDHashNumber
  HashKey(const KeyTypePointer aKey)
  {
    PLDHashNumber hash = 0;
    FloatUint32 convert;
    hash = AddToHash(hash, int(aKey->mBackendType));
    hash = AddToHash(hash, int(aKey->mExtend));
    for (uint32_t i = 0; i < aKey->mStops.Length(); i++) {
      hash = AddToHash(hash, aKey->mStops[i].color.ToABGR());
      
      convert.f = aKey->mStops[i].offset;
      hash = AddToHash(hash, convert.f ? convert.u : 0);
    }
    return hash;
  }

  bool KeyEquals(KeyTypePointer aKey) const
  {
    bool sameStops = true;
    if (aKey->mStops.Length() != mStops.Length()) {
      sameStops = false;
    } else {
      for (uint32_t i = 0; i < mStops.Length(); i++) {
        if (mStops[i].color.ToABGR() != aKey->mStops[i].color.ToABGR() ||
            mStops[i].offset != aKey->mStops[i].offset) {
          sameStops = false;
          break;
        }
      }
    }

    return sameStops &&
           (aKey->mBackendType == mBackendType) &&
           (aKey->mExtend == mExtend);
  }
  static KeyTypePointer KeyToPointer(KeyType aKey)
  {
    return &aKey;
  }
};





struct GradientCacheData {
  GradientCacheData(GradientStops* aStops, const GradientCacheKey& aKey)
    : mStops(aStops),
      mKey(aKey)
  {}

  GradientCacheData(const GradientCacheData& aOther)
    : mStops(aOther.mStops),
      mKey(aOther.mKey)
  { }

  nsExpirationState *GetExpirationState() {
    return &mExpirationState;
  }

  nsExpirationState mExpirationState;
  const RefPtr<GradientStops> mStops;
  GradientCacheKey mKey;
};















class GradientCache MOZ_FINAL : public nsExpirationTracker<GradientCacheData,4>
{
  public:
    GradientCache()
      : nsExpirationTracker<GradientCacheData, 4>(MAX_GENERATION_MS)
    {
      srand(time(nullptr));
      mTimerPeriod = rand() % MAX_GENERATION_MS + 1;
      Telemetry::Accumulate(Telemetry::GRADIENT_RETENTION_TIME, mTimerPeriod);
    }

    virtual void NotifyExpired(GradientCacheData* aObject)
    {
      
      RemoveObject(aObject);
      mHashEntries.Remove(aObject->mKey);
    }

    GradientCacheData* Lookup(const nsTArray<GradientStop>& aStops, ExtendMode aExtend, BackendType aBackendType)
    {
      GradientCacheData* gradient =
        mHashEntries.Get(GradientCacheKey(aStops, aExtend, aBackendType));

      if (gradient) {
        MarkUsed(gradient);
      }

      return gradient;
    }

    
    
    bool RegisterEntry(GradientCacheData* aValue)
    {
      nsresult rv = AddObject(aValue);
      if (NS_FAILED(rv)) {
        
        
        
        
        
        return false;
      }
      mHashEntries.Put(aValue->mKey, aValue);
      return true;
    }

  protected:
    uint32_t mTimerPeriod;
    static const uint32_t MAX_GENERATION_MS = 10000;
    



    nsClassHashtable<GradientCacheKey, GradientCacheData> mHashEntries;
};

static GradientCache* gGradientCache = nullptr;

GradientStops *
gfxGradientCache::GetGradientStops(DrawTarget *aDT, nsTArray<GradientStop>& aStops, ExtendMode aExtend)
{
  if (!gGradientCache) {
    gGradientCache = new GradientCache();
  }
  GradientCacheData* cached =
    gGradientCache->Lookup(aStops, aExtend, aDT->GetBackendType());
  return cached ? cached->mStops : nullptr;
}

GradientStops *
gfxGradientCache::GetOrCreateGradientStops(DrawTarget *aDT, nsTArray<GradientStop>& aStops, ExtendMode aExtend)
{
  RefPtr<GradientStops> gs = GetGradientStops(aDT, aStops, aExtend);
  if (!gs) {
    gs = aDT->CreateGradientStops(aStops.Elements(), aStops.Length(), aExtend);
    if (!gs) {
      return nullptr;
    }
    GradientCacheData *cached =
      new GradientCacheData(gs, GradientCacheKey(aStops, aExtend,
                                                 aDT->GetBackendType()));
    if (!gGradientCache->RegisterEntry(cached)) {
      delete cached;
    }
  }
  return gs;
}

void
gfxGradientCache::PurgeAllCaches()
{
  if (gGradientCache) {
    gGradientCache->AgeAllGenerations();
  }
}

void
gfxGradientCache::Shutdown()
{
  delete gGradientCache;
  gGradientCache = nullptr;
}

}
}
