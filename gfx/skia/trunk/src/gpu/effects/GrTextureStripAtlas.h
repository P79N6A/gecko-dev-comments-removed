






#ifndef GrTextureStripAtlas_DEFINED
#define GrTextureStripAtlas_DEFINED

#include "GrBinHashKey.h"
#include "GrTHashTable.h"
#include "SkBitmap.h"
#include "SkGr.h"
#include "SkTDArray.h"
#include "SkTypes.h"





class GrTextureStripAtlas {
public:
    


    struct Desc {
        Desc() { memset(this, 0, sizeof(*this)); }
        uint16_t fWidth, fHeight, fRowHeight;
        GrPixelConfig fConfig;
        GrContext* fContext;
        const uint32_t* asKey() const { return reinterpret_cast<const uint32_t*>(this); }
    };

    


    static GrTextureStripAtlas* GetAtlas(const Desc& desc);

    ~GrTextureStripAtlas();

    





    int lockRow(const SkBitmap& data);
    void unlockRow(int row);

    















    SkScalar getYOffset(int row) const { return SkIntToScalar(row) / fNumRows; }
    SkScalar getVerticalScaleFactor() const { return SkIntToScalar(fDesc.fRowHeight) / fDesc.fHeight; }

    GrContext* getContext() const { return fDesc.fContext; }
    GrTexture* getTexture() const { return fTexture; }

private:

    
    const static uint32_t kEmptyAtlasRowKey = 0xffffffff;

    



    struct AtlasRow : public SkNoncopyable {
        AtlasRow() : fKey(kEmptyAtlasRowKey), fLocks(0), fNext(NULL), fPrev(NULL) { }
        
        uint32_t fKey;
        
        int32_t fLocks;
        
        AtlasRow* fNext;
        AtlasRow* fPrev;
    };

    


    GrTextureStripAtlas(Desc desc);

    void lockTexture();
    void unlockTexture();

    


    void initLRU();

    


    AtlasRow* getLRU();

    void appendLRU(AtlasRow* row);
    void removeFromLRU(AtlasRow* row);

    



    int searchByKey(uint32_t key);

    


    static bool KeyLess(const AtlasRow& lhs, const AtlasRow& rhs) {
        return lhs.fKey < rhs.fKey;
    }

#ifdef SK_DEBUG
    void validate();
#endif

    



    static void CleanUp(const GrContext* context, void* info);

    
    class AtlasEntry;
    class AtlasHashKey : public GrBinHashKey<sizeof(GrTextureStripAtlas::Desc)> {
    public:
        static bool Equals(const AtlasEntry& entry, const AtlasHashKey& key);
        static bool LessThan(const AtlasEntry& entry, const AtlasHashKey& key);
    };
    class AtlasEntry : public ::SkNoncopyable {
    public:
        AtlasEntry() : fAtlas(NULL) {}
        ~AtlasEntry() { SkDELETE(fAtlas); }
        AtlasHashKey fKey;
        GrTextureStripAtlas* fAtlas;
    };

    static GrTHashTable<AtlasEntry, AtlasHashKey, 8>* gAtlasCache;

    static GrTHashTable<AtlasEntry, AtlasHashKey, 8>* GetCache();

    
    static int32_t gCacheCount;

    
    
    const int32_t fCacheKey;

    
    int32_t fLockedRows;

    const Desc fDesc;
    const uint16_t fNumRows;
    GrTexture* fTexture;

    
    
    
    AtlasRow* fRows;

    
    
    AtlasRow* fLRUFront;
    AtlasRow* fLRUBack;

    
    SkTDArray<AtlasRow*> fKeyTable;
};

inline bool GrTextureStripAtlas::AtlasHashKey::Equals(const AtlasEntry& entry,
                                                      const AtlasHashKey& key) {
    return entry.fKey == key;
}

inline bool GrTextureStripAtlas::AtlasHashKey::LessThan(const AtlasEntry& entry,
                                                        const AtlasHashKey& key) {
    return entry.fKey < key;
}

#endif
