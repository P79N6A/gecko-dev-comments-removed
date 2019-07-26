






#ifndef SkLazyPixelRef_DEFINED
#define SkLazyPixelRef_DEFINED

#include "SkBitmapFactory.h"
#include "SkImage.h"
#include "SkPixelRef.h"
#include "SkFlattenable.h"

class SkColorTable;
class SkData;
class SkImageCache;

#ifdef SK_DEBUG
    #define LAZY_CACHE_STATS 1
#elif !defined(LAZY_CACHE_STATS)
    #define LAZY_CACHE_STATS 0
#endif




class SkLazyPixelRef : public SkPixelRef {

public:
    






    SkLazyPixelRef(SkData*, SkBitmapFactory::DecodeProc, SkImageCache*);

    virtual ~SkLazyPixelRef();

#ifdef SK_DEBUG
    intptr_t getCacheId() const { return fCacheId; }
#endif

#if LAZY_CACHE_STATS
    static int32_t GetCacheHits() { return gCacheHits; }
    static int32_t GetCacheMisses() { return gCacheMisses; }
    static void ResetCacheStats() { gCacheHits = gCacheMisses = 0; }
#endif

    
    
    
    
    SK_DECLARE_UNFLATTENABLE_OBJECT()

protected:
    virtual void* onLockPixels(SkColorTable**) SK_OVERRIDE;
    virtual void onUnlockPixels() SK_OVERRIDE;
    virtual bool onLockPixelsAreWritable() const SK_OVERRIDE { return false; }
    virtual SkData* onRefEncodedData() SK_OVERRIDE;

private:
    bool                        fErrorInDecoding;
    SkData*                     fData;
    SkBitmapFactory::DecodeProc fDecodeProc;
    SkImageCache*               fImageCache;
    intptr_t                    fCacheId;
    size_t                      fRowBytes;

#if LAZY_CACHE_STATS
    static int32_t              gCacheHits;
    static int32_t              gCacheMisses;
#endif

    typedef SkPixelRef INHERITED;
};

#endif  
