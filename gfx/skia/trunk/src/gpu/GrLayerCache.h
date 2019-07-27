






#ifndef GrLayerCache_DEFINED
#define GrLayerCache_DEFINED

#define USE_ATLAS 0

#include "GrAllocPool.h"
#include "GrAtlas.h"
#include "GrPictureUtils.h"
#include "GrRect.h"
#include "SkChecksum.h"
#include "SkTDynamicHash.h"
#include "SkMessageBus.h"

class SkPicture;


struct GrPictureDeletedMessage {
    uint32_t pictureID;
};



struct GrPictureInfo {
public:
    
    static const uint32_t& GetKey(const GrPictureInfo& pictInfo) { return pictInfo.fPictureID; }
    static uint32_t Hash(const uint32_t& key) { return SkChecksum::Mix(key); }

    
    GrPictureInfo(uint32_t pictureID) : fPictureID(pictureID) { }

    const uint32_t fPictureID;

    GrAtlas::ClientPlotUsage  fPlotUsage;
};







struct GrCachedLayer {
public:
    
    struct Key {
        Key(uint32_t pictureID, int layerID) : fPictureID(pictureID) , fLayerID(layerID) {}

        bool operator==(const Key& other) const {
            return fPictureID == other.fPictureID && fLayerID == other.fLayerID;
        }

        uint32_t getPictureID() const { return fPictureID; }
        int      getLayerID() const { return fLayerID; }

    private:
        
        const uint32_t fPictureID;
        
        const int      fLayerID;
    };

    static const Key& GetKey(const GrCachedLayer& layer) { return layer.fKey; }
    static uint32_t Hash(const Key& key) { 
        return SkChecksum::Mix((key.getPictureID() << 16) | key.getLayerID());
    }

    
    GrCachedLayer(uint32_t pictureID, int layerID) 
        : fKey(pictureID, layerID)
        , fTexture(NULL)
        , fRect(GrIRect16::MakeEmpty())
        , fPlot(NULL) {
        SkASSERT(SK_InvalidGenID != pictureID && layerID >= 0);
    }

    uint32_t pictureID() const { return fKey.getPictureID(); }
    int layerID() const { return fKey.getLayerID(); }

    
    void setTexture(GrTexture* texture, const GrIRect16& rect) {
        if (NULL != fTexture) {
            fTexture->unref();
        }

        fTexture = texture; 
        fRect = rect;
    }
    GrTexture* texture() { return fTexture; }
    const GrIRect16& rect() const { return fRect; }

    void setPlot(GrPlot* plot) {
        SkASSERT(NULL == fPlot);
        fPlot = plot;
    }
    GrPlot* plot() { return fPlot; }

    bool isAtlased() const { return NULL != fPlot; }

    SkDEBUGCODE(void validate(const GrTexture* backingTexture) const;)

private:
    const Key       fKey;

    
    
    
    GrTexture*      fTexture;

    
    
    
    GrIRect16       fRect;

    
    
    GrPlot*         fPlot;
};








class GrLayerCache {
public:
    GrLayerCache(GrContext*);
    ~GrLayerCache();

    
    
    void freeAll();

    GrCachedLayer* findLayer(const SkPicture* picture, int layerID);
    GrCachedLayer* findLayerOrCreate(const SkPicture* picture, int layerID);
    
    
    
    
    
    bool lock(GrCachedLayer* layer, const GrTextureDesc& desc);

    
    void unlock(GrCachedLayer* layer);

    
    void trackPicture(const SkPicture* picture);

    
    void processDeletedPictures();

    SkDEBUGCODE(void validate() const;)

private:
    static const int kNumPlotsX = 2;
    static const int kNumPlotsY = 2;

    GrContext*                fContext;  
    SkAutoTDelete<GrAtlas>    fAtlas;    

    
    
    
    
    
    SkTDynamicHash<GrPictureInfo, uint32_t> fPictureHash;

    SkTDynamicHash<GrCachedLayer, GrCachedLayer::Key> fLayerHash;

    SkMessageBus<GrPictureDeletedMessage>::Inbox fPictDeletionInbox;

    SkAutoTUnref<SkPicture::DeletionListener> fDeletionListener;

    void initAtlas();
    GrCachedLayer* createLayer(const SkPicture* picture, int layerID);

    
    void purge(uint32_t pictureID);

    
    friend class TestingAccess;
    int numLayers() const { return fLayerHash.count(); }
};

#endif
