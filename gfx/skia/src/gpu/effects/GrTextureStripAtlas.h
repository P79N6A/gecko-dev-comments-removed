







#ifndef GrTextureStripAtlas_DEFINED
#define GrTextureStripAtlas_DEFINED

#include "SkBitmap.h"
#include "GrTHashCache.h"
#include "SkGr.h"
#include "SkTDArray.h"





class GrTextureStripAtlas {
public:
    GR_DECLARE_RESOURCE_CACHE_DOMAIN(GetTextureStripAtlasDomain)

    


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

    















    GrScalar getYOffset(int row) const { return SkIntToScalar(row) / fNumRows; }
    GrScalar getVerticalScaleFactor() const { return SkIntToScalar(fDesc.fRowHeight) / fDesc.fHeight; }

    GrContext* getContext() const { return fDesc.fContext; }
    GrTexture* getTexture() const { return fTexture; }

private:

    
    const static uint32_t kEmptyAtlasRowKey = 0xffffffff;

    



    struct AtlasRow : public GrNoncopyable {
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

    


    static int compareKeys(const AtlasRow* lhs, const AtlasRow* rhs) {
        return lhs->fKey - rhs->fKey;
    }

#ifdef SK_DEBUG
    void validate();
#endif

    
    static int32_t gCacheCount;

    
    
    const uint64_t fCacheID;

    
    int32_t fLockedRows;

    const Desc fDesc;
    const uint16_t fNumRows;
    GrTexture* fTexture;

    
    
    
    AtlasRow* fRows;

    
    
    AtlasRow* fLRUFront;
    AtlasRow* fLRUBack;

    
    SkTDArray<AtlasRow*> fKeyTable;
};

#endif

