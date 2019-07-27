








#ifndef SkGlyphCache_DEFINED
#define SkGlyphCache_DEFINED

#include "SkBitmap.h"
#include "SkChunkAlloc.h"
#include "SkDescriptor.h"
#include "SkGlyph.h"
#include "SkScalerContext.h"
#include "SkTemplates.h"
#include "SkTDArray.h"

struct SkDeviceProperties;
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

#ifdef SK_BUILD_FOR_ANDROID
    

    unsigned getBaseGlyphCount(SkUnichar charCode) const {
        return fScalerContext->getBaseGlyphCount(charCode);
    }
#endif

    


    const void* findImage(const SkGlyph&);
    


    const SkPath* findPath(const SkGlyph&);
    


    const void* findDistanceField(const SkGlyph&);

    

    const SkPaint::FontMetrics& getFontMetrics() const {
        return fFontMetrics;
    }

    const SkDescriptor& getDescriptor() const { return *fDesc; }

    SkMask::Format getMaskFormat() const {
        return fScalerContext->getMaskFormat();
    }

    bool isSubpixel() const {
        return fScalerContext->isSubpixel();
    }

    








    
    bool getAuxProcData(void (*auxProc)(void*), void** dataPtr) const;
    
    void setAuxProc(void (*auxProc)(void*), void* auxData);

    SkScalerContext* getScalerContext() const { return fScalerContext; }

    



    static void VisitAllCaches(bool (*proc)(SkGlyphCache*, void*), void* ctx);

    



    static SkGlyphCache* VisitCache(SkTypeface*, const SkDescriptor* desc,
                                    bool (*proc)(const SkGlyphCache*, void*),
                                    void* context);

    



    static void AttachCache(SkGlyphCache*);

    








    static SkGlyphCache* DetachCache(SkTypeface* typeface,
                                     const SkDescriptor* desc) {
        return VisitCache(typeface, desc, DetachProc, NULL);
    }

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
    
    SkGlyphCache(SkTypeface*, const SkDescriptor*, SkScalerContext*);
    ~SkGlyphCache();

    enum MetricsType {
        kJustAdvance_MetricsType,
        kFull_MetricsType
    };

    SkGlyph* lookupMetrics(uint32_t id, MetricsType);
    static bool DetachProc(const SkGlyphCache*, void*) { return true; }

    SkGlyphCache*       fNext, *fPrev;
    SkDescriptor*       fDesc;
    SkScalerContext*    fScalerContext;
    SkPaint::FontMetrics fFontMetrics;

    enum {
        kHashBits   = 8,
        kHashCount  = 1 << kHashBits,
        kHashMask   = kHashCount - 1
    };
    SkGlyph*            fGlyphHash[kHashCount];
    SkTDArray<SkGlyph*> fGlyphArray;
    SkChunkAlloc        fGlyphAlloc;

    struct CharGlyphRec {
        uint32_t    fID;    
        SkGlyph*    fGlyph;
    };
    
    CharGlyphRec    fCharToGlyphHash[kHashCount];

    static inline unsigned ID2HashIndex(uint32_t id) {
        id ^= id >> 16;
        id ^= id >> 8;
        return id & kHashMask;
    }

    
    size_t  fMemoryUsed;

    struct AuxProcRec {
        AuxProcRec* fNext;
        void (*fProc)(void*);
        void* fData;
    };
    AuxProcRec* fAuxProcList;
    void invokeAndRemoveAuxProcs();

    inline static SkGlyphCache* FindTail(SkGlyphCache* head);

    friend class SkGlyphCache_Globals;
};

class SkAutoGlyphCacheBase {
public:
    SkGlyphCache* getCache() const { return fCache; }

    void release() {
        if (fCache) {
            SkGlyphCache::AttachCache(fCache);
            fCache = NULL;
        }
    }

protected:
    
    
    SkAutoGlyphCacheBase(SkGlyphCache* cache) : fCache(cache) {}
    SkAutoGlyphCacheBase(SkTypeface* typeface, const SkDescriptor* desc) {
        fCache = SkGlyphCache::DetachCache(typeface, desc);
    }
    SkAutoGlyphCacheBase(const SkPaint& paint,
                         const SkDeviceProperties* deviceProperties,
                         const SkMatrix* matrix) {
        fCache = NULL;
    }
    SkAutoGlyphCacheBase() {
        fCache = NULL;
    }
    ~SkAutoGlyphCacheBase() {
        if (fCache) {
            SkGlyphCache::AttachCache(fCache);
        }
    }

    SkGlyphCache*   fCache;

private:
    static bool DetachProc(const SkGlyphCache*, void*);
};

class SkAutoGlyphCache : public SkAutoGlyphCacheBase {
public:
    SkAutoGlyphCache(SkGlyphCache* cache) : SkAutoGlyphCacheBase(cache) {}
    SkAutoGlyphCache(SkTypeface* typeface, const SkDescriptor* desc) :
        SkAutoGlyphCacheBase(typeface, desc) {}
    SkAutoGlyphCache(const SkPaint& paint,
                     const SkDeviceProperties* deviceProperties,
                     const SkMatrix* matrix) {
        fCache = paint.detachCache(deviceProperties, matrix, false);
    }

private:
    SkAutoGlyphCache() : SkAutoGlyphCacheBase() {}
};
#define SkAutoGlyphCache(...) SK_REQUIRE_LOCAL_VAR(SkAutoGlyphCache)

class SkAutoGlyphCacheNoGamma : public SkAutoGlyphCacheBase {
public:
    SkAutoGlyphCacheNoGamma(SkGlyphCache* cache) : SkAutoGlyphCacheBase(cache) {}
    SkAutoGlyphCacheNoGamma(SkTypeface* typeface, const SkDescriptor* desc) :
        SkAutoGlyphCacheBase(typeface, desc) {}
    SkAutoGlyphCacheNoGamma(const SkPaint& paint,
                            const SkDeviceProperties* deviceProperties,
                            const SkMatrix* matrix) {
        fCache = paint.detachCache(deviceProperties, matrix, true);
    }

private:
    SkAutoGlyphCacheNoGamma() : SkAutoGlyphCacheBase() {}
};
#define SkAutoGlyphCacheNoGamma(...) SK_REQUIRE_LOCAL_VAR(SkAutoGlyphCacheNoGamma)

#endif
