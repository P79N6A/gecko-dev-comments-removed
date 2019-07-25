








#include "SkGlyphCache.h"
#include "SkGraphics.h"
#include "SkPaint.h"
#include "SkTemplates.h"

#define SPEW_PURGE_STATUS



bool gSkSuppressFontCachePurgeSpew;



#ifdef RECORD_HASH_EFFICIENCY
    static uint32_t gHashSuccess;
    static uint32_t gHashCollision;

    static void RecordHashSuccess() {
        gHashSuccess += 1;
    }

    static void RecordHashCollisionIf(bool pred) {
        if (pred) {
            gHashCollision += 1;

            uint32_t total = gHashSuccess + gHashCollision;
            SkDebugf("Font Cache Hash success rate: %d%%\n",
                     100 * gHashSuccess / total);
        }
    }
#else
    #define RecordHashSuccess() (void)0
    #define RecordHashCollisionIf(pred) (void)0
#endif
#define RecordHashCollision() RecordHashCollisionIf(true)



#define kMinGlphAlloc       (sizeof(SkGlyph) * 64)
#define kMinImageAlloc      (24 * 64)   // should be pointsize-dependent

#define METRICS_RESERVE_COUNT  128  // so we don't grow this array a lot

SkGlyphCache::SkGlyphCache(const SkDescriptor* desc)
        : fGlyphAlloc(kMinGlphAlloc), fImageAlloc(kMinImageAlloc) {
    fPrev = fNext = NULL;

    fDesc = desc->copy();
    fScalerContext = SkScalerContext::Create(desc);
    fScalerContext->getFontMetrics(NULL, &fFontMetricsY);

    
    memset(fGlyphHash, 0, sizeof(fGlyphHash));
    
    memset(fCharToGlyphHash, 0xFF, sizeof(fCharToGlyphHash));

    fMemoryUsed = sizeof(*this) + kMinGlphAlloc + kMinImageAlloc;

    fGlyphArray.setReserve(METRICS_RESERVE_COUNT);

    fMetricsCount = 0;
    fAdvanceCount = 0;
    fAuxProcList = NULL;
}

SkGlyphCache::~SkGlyphCache() {
    SkGlyph**   gptr = fGlyphArray.begin();
    SkGlyph**   stop = fGlyphArray.end();
    while (gptr < stop) {
        SkPath* path = (*gptr)->fPath;
        if (path) {
            SkDELETE(path);
        }
        gptr += 1;
    }
    SkDescriptor::Free(fDesc);
    SkDELETE(fScalerContext);
    this->invokeAndRemoveAuxProcs();
}



#ifdef SK_DEBUG
#define VALIDATE()  AutoValidate av(this)
#else
#define VALIDATE()
#endif

uint16_t SkGlyphCache::unicharToGlyph(SkUnichar charCode) {
    VALIDATE();
    uint32_t id = SkGlyph::MakeID(charCode);
    const CharGlyphRec& rec = fCharToGlyphHash[ID2HashIndex(id)];

    if (rec.fID == id) {
        return rec.fGlyph->getGlyphID();
    } else {
        return fScalerContext->charToGlyphID(charCode);
    }
}

SkUnichar SkGlyphCache::glyphToUnichar(uint16_t glyphID) {
    return fScalerContext->glyphIDToChar(glyphID);
}

unsigned SkGlyphCache::getGlyphCount() {
    return fScalerContext->getGlyphCount();
}



const SkGlyph& SkGlyphCache::getUnicharAdvance(SkUnichar charCode) {
    VALIDATE();
    uint32_t id = SkGlyph::MakeID(charCode);
    CharGlyphRec* rec = &fCharToGlyphHash[ID2HashIndex(id)];

    if (rec->fID != id) {
        
        rec->fID = id;
        
        id = SkGlyph::MakeID(fScalerContext->charToGlyphID(charCode));
        rec->fGlyph = this->lookupMetrics(id, kJustAdvance_MetricsType);
    }
    return *rec->fGlyph;
}

const SkGlyph& SkGlyphCache::getGlyphIDAdvance(uint16_t glyphID) {
    VALIDATE();
    uint32_t id = SkGlyph::MakeID(glyphID);
    unsigned index = ID2HashIndex(id);
    SkGlyph* glyph = fGlyphHash[index];

    if (NULL == glyph || glyph->fID != id) {
        glyph = this->lookupMetrics(glyphID, kJustAdvance_MetricsType);
        fGlyphHash[index] = glyph;
    }
    return *glyph;
}



const SkGlyph& SkGlyphCache::getUnicharMetrics(SkUnichar charCode) {
    VALIDATE();
    uint32_t id = SkGlyph::MakeID(charCode);
    CharGlyphRec* rec = &fCharToGlyphHash[ID2HashIndex(id)];

    if (rec->fID != id) {
        RecordHashCollisionIf(rec->fGlyph != NULL);
        
        rec->fID = id;
        
        id = SkGlyph::MakeID(fScalerContext->charToGlyphID(charCode));
        rec->fGlyph = this->lookupMetrics(id, kFull_MetricsType);
    } else {
        RecordHashSuccess();
        if (rec->fGlyph->isJustAdvance()) {
            fScalerContext->getMetrics(rec->fGlyph);
        }
    }
    SkASSERT(rec->fGlyph->isFullMetrics());
    return *rec->fGlyph;
}

const SkGlyph& SkGlyphCache::getUnicharMetrics(SkUnichar charCode,
                                               SkFixed x, SkFixed y) {
    VALIDATE();
    uint32_t id = SkGlyph::MakeID(charCode, x, y);
    CharGlyphRec* rec = &fCharToGlyphHash[ID2HashIndex(id)];

    if (rec->fID != id) {
        RecordHashCollisionIf(rec->fGlyph != NULL);
        
        rec->fID = id;
        
        id = SkGlyph::MakeID(fScalerContext->charToGlyphID(charCode), x, y);
        rec->fGlyph = this->lookupMetrics(id, kFull_MetricsType);
    } else {
        RecordHashSuccess();
        if (rec->fGlyph->isJustAdvance()) {
            fScalerContext->getMetrics(rec->fGlyph);
        }
    }
    SkASSERT(rec->fGlyph->isFullMetrics());
    return *rec->fGlyph;
}

const SkGlyph& SkGlyphCache::getGlyphIDMetrics(uint16_t glyphID) {
    VALIDATE();
    uint32_t id = SkGlyph::MakeID(glyphID);
    unsigned index = ID2HashIndex(id);
    SkGlyph* glyph = fGlyphHash[index];

    if (NULL == glyph || glyph->fID != id) {
        RecordHashCollisionIf(glyph != NULL);
        glyph = this->lookupMetrics(glyphID, kFull_MetricsType);
        fGlyphHash[index] = glyph;
    } else {
        RecordHashSuccess();
        if (glyph->isJustAdvance()) {
            fScalerContext->getMetrics(glyph);
        }
    }
    SkASSERT(glyph->isFullMetrics());
    return *glyph;
}

const SkGlyph& SkGlyphCache::getGlyphIDMetrics(uint16_t glyphID,
                                               SkFixed x, SkFixed y) {
    VALIDATE();
    uint32_t id = SkGlyph::MakeID(glyphID, x, y);
    unsigned index = ID2HashIndex(id);
    SkGlyph* glyph = fGlyphHash[index];

    if (NULL == glyph || glyph->fID != id) {
        RecordHashCollisionIf(glyph != NULL);
        glyph = this->lookupMetrics(id, kFull_MetricsType);
        fGlyphHash[index] = glyph;
    } else {
        RecordHashSuccess();
        if (glyph->isJustAdvance()) {
            fScalerContext->getMetrics(glyph);
        }
    }
    SkASSERT(glyph->isFullMetrics());
    return *glyph;
}

SkGlyph* SkGlyphCache::lookupMetrics(uint32_t id, MetricsType mtype) {
    SkGlyph* glyph;

    int     hi = 0;
    int     count = fGlyphArray.count();

    if (count) {
        SkGlyph**   gptr = fGlyphArray.begin();
        int     lo = 0;

        hi = count - 1;
        while (lo < hi) {
            int mid = (hi + lo) >> 1;
            if (gptr[mid]->fID < id) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        glyph = gptr[hi];
        if (glyph->fID == id) {
            if (kFull_MetricsType == mtype && glyph->isJustAdvance()) {
                fScalerContext->getMetrics(glyph);
            }
            return glyph;
        }

        
        if (glyph->fID < id) {
            hi += 1;
        }
    }

    
    fMemoryUsed += sizeof(SkGlyph);

    glyph = (SkGlyph*)fGlyphAlloc.alloc(sizeof(SkGlyph),
                                        SkChunkAlloc::kThrow_AllocFailType);
    glyph->init(id);
    *fGlyphArray.insert(hi) = glyph;

    if (kJustAdvance_MetricsType == mtype) {
        fScalerContext->getAdvance(glyph);
        fAdvanceCount += 1;
    } else {
        SkASSERT(kFull_MetricsType == mtype);
        fScalerContext->getMetrics(glyph);
        fMetricsCount += 1;
    }

    return glyph;
}

const void* SkGlyphCache::findImage(const SkGlyph& glyph) {
    if (glyph.fWidth > 0 && glyph.fWidth < kMaxGlyphWidth) {
        if (glyph.fImage == NULL) {
            size_t  size = glyph.computeImageSize();
            const_cast<SkGlyph&>(glyph).fImage = fImageAlloc.alloc(size,
                                        SkChunkAlloc::kReturnNil_AllocFailType);
            
            if (glyph.fImage) {
                fScalerContext->getImage(glyph);
                
                
                
                
                fMemoryUsed += size;
            }
        }
    }
    return glyph.fImage;
}

const SkPath* SkGlyphCache::findPath(const SkGlyph& glyph) {
    if (glyph.fWidth) {
        if (glyph.fPath == NULL) {
            const_cast<SkGlyph&>(glyph).fPath = SkNEW(SkPath);
            fScalerContext->getPath(glyph, glyph.fPath);
            fMemoryUsed += sizeof(SkPath) +
                    glyph.fPath->getPoints(NULL, 0x7FFFFFFF) * sizeof(SkPoint);
        }
    }
    return glyph.fPath;
}



bool SkGlyphCache::getAuxProcData(void (*proc)(void*), void** dataPtr) const {
    const AuxProcRec* rec = fAuxProcList;
    while (rec) {
        if (rec->fProc == proc) {
            if (dataPtr) {
                *dataPtr = rec->fData;
            }
            return true;
        }
        rec = rec->fNext;
    }
    return false;
}

void SkGlyphCache::setAuxProc(void (*proc)(void*), void* data) {
    if (proc == NULL) {
        return;
    }

    AuxProcRec* rec = fAuxProcList;
    while (rec) {
        if (rec->fProc == proc) {
            rec->fData = data;
            return;
        }
        rec = rec->fNext;
    }
    
    rec = SkNEW(AuxProcRec);
    rec->fProc = proc;
    rec->fData = data;
    rec->fNext = fAuxProcList;
    fAuxProcList = rec;
}

void SkGlyphCache::removeAuxProc(void (*proc)(void*)) {
    AuxProcRec* rec = fAuxProcList;
    AuxProcRec* prev = NULL;
    while (rec) {
        AuxProcRec* next = rec->fNext;
        if (rec->fProc == proc) {
            if (prev) {
                prev->fNext = next;
            } else {
                fAuxProcList = next;
            }
            SkDELETE(rec);
            return;
        }
        prev = rec;
        rec = next;
    }
}

void SkGlyphCache::invokeAndRemoveAuxProcs() {
    AuxProcRec* rec = fAuxProcList;
    while (rec) {
        rec->fProc(rec->fData);
        AuxProcRec* next = rec->fNext;
        SkDELETE(rec);
        rec = next;
    }
}




#ifdef USE_CACHE_HASH
    #define HASH_BITCOUNT   6
    #define HASH_COUNT      (1 << HASH_BITCOUNT)
    #define HASH_MASK       (HASH_COUNT - 1)

    static unsigned desc_to_hashindex(const SkDescriptor* desc)
    {
        SkASSERT(HASH_MASK < 256);  

        uint32_t n = *(const uint32_t*)desc;    
        SkASSERT(n == desc->getChecksum());

        
        n ^= (n >> 24) ^ (n >> 16) ^ (n >> 8) ^ (n >> 30);

        return n & HASH_MASK;
    }
#endif

#include "SkThread.h"

class SkGlyphCache_Globals {
public:
    SkGlyphCache_Globals() {
        fHead = NULL;
        fTotalMemoryUsed = 0;
#ifdef USE_CACHE_HASH
        sk_bzero(fHash, sizeof(fHash));
#endif
    }

    SkMutex         fMutex;
    SkGlyphCache*   fHead;
    size_t          fTotalMemoryUsed;
#ifdef USE_CACHE_HASH
    SkGlyphCache*   fHash[HASH_COUNT];
#endif

#ifdef SK_DEBUG
    void validate() const;
#else
    void validate() const {}
#endif
};

static SkGlyphCache_Globals& getGlobals() {
    
    static SkGlyphCache_Globals* gGlobals = new SkGlyphCache_Globals;
    return *gGlobals;
}

void SkGlyphCache::VisitAllCaches(bool (*proc)(SkGlyphCache*, void*),
                                  void* context) {
    SkGlyphCache_Globals& globals = getGlobals();
    SkAutoMutexAcquire    ac(globals.fMutex);
    SkGlyphCache*         cache;

    globals.validate();

    for (cache = globals.fHead; cache != NULL; cache = cache->fNext) {
        if (proc(cache, context)) {
            break;
        }
    }

    globals.validate();
}







SkGlyphCache* SkGlyphCache::VisitCache(const SkDescriptor* desc,
                              bool (*proc)(const SkGlyphCache*, void*),
                              void* context) {
    SkASSERT(desc);

    SkGlyphCache_Globals& globals = getGlobals();
    SkAutoMutexAcquire    ac(globals.fMutex);
    SkGlyphCache*         cache;
    bool                  insideMutex = true;

    globals.validate();

#ifdef USE_CACHE_HASH
    SkGlyphCache** hash = globals.fHash;
    unsigned index = desc_to_hashindex(desc);
    cache = hash[index];
    if (cache && *cache->fDesc == *desc) {
        cache->detach(&globals.fHead);
        goto FOUND_IT;
    }
#endif

    for (cache = globals.fHead; cache != NULL; cache = cache->fNext) {
        if (cache->fDesc->equals(*desc)) {
            cache->detach(&globals.fHead);
            goto FOUND_IT;
        }
    }

    


    ac.release();           
    insideMutex = false;    

    cache = SkNEW_ARGS(SkGlyphCache, (desc));

FOUND_IT:

    AutoValidate av(cache);

    if (proc(cache, context)) {   
        if (insideMutex) {
            SkASSERT(globals.fTotalMemoryUsed >= cache->fMemoryUsed);
            globals.fTotalMemoryUsed -= cache->fMemoryUsed;
#ifdef USE_CACHE_HASH
            hash[index] = NULL;
#endif
        }
    } else {                        
        if (insideMutex) {
            cache->attachToHead(&globals.fHead);
#ifdef USE_CACHE_HASH
            hash[index] = cache;
#endif
        } else {
            AttachCache(cache);
        }
        cache = NULL;
    }
    return cache;
}

void SkGlyphCache::AttachCache(SkGlyphCache* cache) {
    SkASSERT(cache);
    SkASSERT(cache->fNext == NULL);

    SkGlyphCache_Globals& globals = getGlobals();
    SkAutoMutexAcquire    ac(globals.fMutex);

    globals.validate();
    cache->validate();

    
    {
        size_t allocated = globals.fTotalMemoryUsed + cache->fMemoryUsed;
        size_t budgeted = SkGraphics::GetFontCacheLimit();
        if (allocated > budgeted) {
            (void)InternalFreeCache(&globals, allocated - budgeted);
        }
    }

    cache->attachToHead(&globals.fHead);
    globals.fTotalMemoryUsed += cache->fMemoryUsed;

#ifdef USE_CACHE_HASH
    unsigned index = desc_to_hashindex(cache->fDesc);
    SkASSERT(globals.fHash[index] != cache);
    globals.fHash[index] = cache;
#endif

    globals.validate();
}

size_t SkGlyphCache::GetCacheUsed() {
    SkGlyphCache_Globals& globals = getGlobals();
    SkAutoMutexAcquire  ac(globals.fMutex);

    return SkGlyphCache::ComputeMemoryUsed(globals.fHead);
}

bool SkGlyphCache::SetCacheUsed(size_t bytesUsed) {
    size_t curr = SkGlyphCache::GetCacheUsed();

    if (curr > bytesUsed) {
        SkGlyphCache_Globals& globals = getGlobals();
        SkAutoMutexAcquire  ac(globals.fMutex);

        return InternalFreeCache(&globals, curr - bytesUsed) > 0;
    }
    return false;
}



SkGlyphCache* SkGlyphCache::FindTail(SkGlyphCache* cache) {
    if (cache) {
        while (cache->fNext) {
            cache = cache->fNext;
        }
    }
    return cache;
}

size_t SkGlyphCache::ComputeMemoryUsed(const SkGlyphCache* head) {
    size_t size = 0;

    while (head != NULL) {
        size += head->fMemoryUsed;
        head = head->fNext;
    }
    return size;
}

#ifdef SK_DEBUG
void SkGlyphCache_Globals::validate() const {
    size_t computed = SkGlyphCache::ComputeMemoryUsed(fHead);
    if (fTotalMemoryUsed != computed) {
        printf("total %d, computed %d\n", (int)fTotalMemoryUsed, (int)computed);
    }
    SkASSERT(fTotalMemoryUsed == computed);
}
#endif

size_t SkGlyphCache::InternalFreeCache(SkGlyphCache_Globals* globals,
                                       size_t bytesNeeded) {
    globals->validate();

    size_t  bytesFreed = 0;
    int     count = 0;

    
    size_t minToPurge = globals->fTotalMemoryUsed >> 2;
    if (bytesNeeded < minToPurge)
        bytesNeeded = minToPurge;

    SkGlyphCache* cache = FindTail(globals->fHead);
    while (cache != NULL && bytesFreed < bytesNeeded) {
        SkGlyphCache* prev = cache->fPrev;
        bytesFreed += cache->fMemoryUsed;

#ifdef USE_CACHE_HASH
        unsigned index = desc_to_hashindex(cache->fDesc);
        if (cache == globals->fHash[index]) {
            globals->fHash[index] = NULL;
        }
#endif

        cache->detach(&globals->fHead);
        SkDELETE(cache);
        cache = prev;
        count += 1;
    }

    SkASSERT(bytesFreed <= globals->fTotalMemoryUsed);
    globals->fTotalMemoryUsed -= bytesFreed;
    globals->validate();

#ifdef SPEW_PURGE_STATUS
    if (count && !gSkSuppressFontCachePurgeSpew) {
        SkDebugf("purging %dK from font cache [%d entries]\n",
                 (int)(bytesFreed >> 10), count);
    }
#endif

    return bytesFreed;
}


#ifdef SK_DEBUG

void SkGlyphCache::validate() const {
    int count = fGlyphArray.count();
    for (int i = 0; i < count; i++) {
        const SkGlyph* glyph = fGlyphArray[i];
        SkASSERT(glyph);
        SkASSERT(fGlyphAlloc.contains(glyph));
        if (glyph->fImage) {
            SkASSERT(fImageAlloc.contains(glyph->fImage));
        }
    }
}

#endif
