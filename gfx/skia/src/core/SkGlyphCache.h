








#ifndef SkGlyphCache_DEFINED
#define SkGlyphCache_DEFINED

#include "SkBitmap.h"
#include "SkChunkAlloc.h"
#include "SkDescriptor.h"
#include "SkScalerContext.h"
#include "SkTemplates.h"

class SkPaint;

class SkGlyphCache_Globals;












class SkGlyphCache {
public:
    



    const SkGlyph& getUnicharAdvance(SkUnichar);
    const SkGlyph& getGlyphIDAdvance(uint16_t);

    






    const SkGlyph& getUnicharMetrics(SkUnichar);
    const SkGlyph& getGlyphIDMetrics(uint16_t);

    




    const SkGlyph& getUnicharMetrics(SkUnichar, SkFixed x, SkFixed y);
    const SkGlyph& getGlyphIDMetrics(uint16_t, SkFixed x, SkFixed y);

    



    uint16_t unicharToGlyph(SkUnichar);

    


    SkUnichar glyphToUnichar(uint16_t);

    

    unsigned getGlyphCount();

    


    const void* findImage(const SkGlyph&);
    


    const SkPath* findPath(const SkGlyph&);

    

    const SkPaint::FontMetrics& getFontMetricsY() const {
        return fFontMetricsY;
    }

    const SkDescriptor& getDescriptor() const { return *fDesc; }

    SkMask::Format getMaskFormat() const {
        return fScalerContext->getMaskFormat();
    }

    








    
    bool getAuxProcData(void (*auxProc)(void*), void** dataPtr) const;
    
    void setAuxProc(void (*auxProc)(void*), void* auxData);
    
    
    void removeAuxProc(void (*auxProc)(void*));

    



    static void VisitAllCaches(bool (*proc)(SkGlyphCache*, void*), void* ctx);

    



    static SkGlyphCache* VisitCache(const SkDescriptor* desc,
                                    bool (*proc)(const SkGlyphCache*, void*),
                                    void* context);

    



    static void AttachCache(SkGlyphCache*);

    








    static SkGlyphCache* DetachCache(const SkDescriptor* desc) {
        return VisitCache(desc, DetachProc, NULL);
    }

    

    static size_t GetCacheUsed();

    





    static bool SetCacheUsed(size_t bytesUsed);

#ifdef SK_DEBUG
    void validate() const;
#else
    void validate() const {}
#endif

    class AutoValidate : SkNoncopyable {
    public:
        AutoValidate(const SkGlyphCache* cache) : fCache(cache) {
            if (fCache) {
                fCache->validate();
            }
        }
        ~AutoValidate() {
            if (fCache) {
                fCache->validate();
            }
        }
        void forget() {
            fCache = NULL;
        }
    private:
        const SkGlyphCache* fCache;
    };

private:
    SkGlyphCache(const SkDescriptor*);
    ~SkGlyphCache();

    enum MetricsType {
        kJustAdvance_MetricsType,
        kFull_MetricsType
    };

    SkGlyph* lookupMetrics(uint32_t id, MetricsType);
    static bool DetachProc(const SkGlyphCache*, void*) { return true; }

    void detach(SkGlyphCache** head) {
        if (fPrev) {
            fPrev->fNext = fNext;
        } else {
            *head = fNext;
        }
        if (fNext) {
            fNext->fPrev = fPrev;
        }
        fPrev = fNext = NULL;
    }

    void attachToHead(SkGlyphCache** head) {
        SkASSERT(NULL == fPrev && NULL == fNext);
        if (*head) {
            (*head)->fPrev = this;
            fNext = *head;
        }
        *head = this;
    }

    SkGlyphCache*       fNext, *fPrev;
    SkDescriptor*       fDesc;
    SkScalerContext*    fScalerContext;
    SkPaint::FontMetrics fFontMetricsY;

    enum {
        kHashBits   = 8,
        kHashCount  = 1 << kHashBits,
        kHashMask   = kHashCount - 1
    };
    SkGlyph*            fGlyphHash[kHashCount];
    SkTDArray<SkGlyph*> fGlyphArray;
    SkChunkAlloc        fGlyphAlloc;
    SkChunkAlloc        fImageAlloc;

    int fMetricsCount, fAdvanceCount;

    struct CharGlyphRec {
        uint32_t    fID;    
        SkGlyph*    fGlyph;
    };
    
    CharGlyphRec    fCharToGlyphHash[kHashCount];

    enum {
        
        kShiftForHashIndex = SkGlyph::kSubShift +
                             SkGlyph::kSubBits*2 -
                             kHashBits
    };

    static inline unsigned ID2HashIndex(uint32_t id) {
        return (id ^ (id >> kShiftForHashIndex)) & kHashMask;
    }

    
    size_t  fMemoryUsed;

    struct AuxProcRec {
        AuxProcRec* fNext;
        void (*fProc)(void*);
        void* fData;
    };
    AuxProcRec* fAuxProcList;
    void invokeAndRemoveAuxProcs();

    
    static size_t InternalFreeCache(SkGlyphCache_Globals*, size_t bytesNeeded);

    inline static SkGlyphCache* FindTail(SkGlyphCache* head);
    static size_t ComputeMemoryUsed(const SkGlyphCache* head);

    friend class SkGlyphCache_Globals;
};

class SkAutoGlyphCache {
public:
    SkAutoGlyphCache(SkGlyphCache* cache) : fCache(cache) {}
    SkAutoGlyphCache(const SkDescriptor* desc) {
        fCache = SkGlyphCache::DetachCache(desc);
    }
    SkAutoGlyphCache(const SkPaint& paint, const SkMatrix* matrix) {
        fCache = paint.detachCache(matrix);
    }
    ~SkAutoGlyphCache() {
        if (fCache) {
            SkGlyphCache::AttachCache(fCache);
        }
    }

    SkGlyphCache* getCache() const { return fCache; }

    void release() {
        if (fCache) {
            SkGlyphCache::AttachCache(fCache);
            fCache = NULL;
        }
    }

private:
    SkGlyphCache*   fCache;

    static bool DetachProc(const SkGlyphCache*, void*);
};

#endif

