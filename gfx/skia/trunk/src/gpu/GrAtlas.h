







#ifndef GrAtlas_DEFINED
#define GrAtlas_DEFINED


#include "GrTexture.h"
#include "GrDrawTarget.h"
#include "SkPoint.h"
#include "SkTInternalLList.h"

class GrGpu;
class GrRectanizer;
class GrAtlas;











class GrPlot {
public:
    SK_DECLARE_INTERNAL_LLIST_INTERFACE(GrPlot);

    
    
    int id() const { return fID; }

    GrTexture* texture() const { return fTexture; }

    bool addSubImage(int width, int height, const void*, SkIPoint16*);

    GrDrawTarget::DrawToken drawToken() const { return fDrawToken; }
    void setDrawToken(GrDrawTarget::DrawToken draw) { fDrawToken = draw; }

    void uploadToTexture();

    void resetRects();

private:
    GrPlot();
    ~GrPlot(); 
    void init(GrAtlas* atlas, int id, int offX, int offY, int width, int height, size_t bpp,
              bool batchUploads);

    
    GrDrawTarget::DrawToken fDrawToken;

    int                     fID;
    unsigned char*          fPlotData;
    GrTexture*              fTexture;
    GrRectanizer*           fRects;
    GrAtlas*                fAtlas;
    SkIPoint16              fOffset;        
    size_t                  fBytesPerPixel;
    SkIRect                 fDirtyRect;
    bool                    fDirty;
    bool                    fBatchUploads;

    friend class GrAtlas;
};

typedef SkTInternalLList<GrPlot> GrPlotList;

class GrAtlas {
public:
    
    
    class ClientPlotUsage {
    public:
        bool isEmpty() const { return 0 == fPlots.count(); }

    private:
        SkTDArray<GrPlot*> fPlots;

        friend class GrAtlas;
    };

    GrAtlas(GrGpu*, GrPixelConfig, GrTextureFlags flags, 
            const SkISize& backingTextureSize,
            int numPlotsX, int numPlotsY, bool batchUploads);
    ~GrAtlas();

    
    
    
    
    
    GrPlot* addToAtlas(ClientPlotUsage*, int width, int height, const void* image, SkIPoint16* loc);

    
    static void RemovePlot(ClientPlotUsage* usage, const GrPlot* plot);

    
    
    GrPlot* getUnusedPlot();

    GrTexture* getTexture() const {
        return fTexture;
    }

    void uploadPlotsToTexture();

private:
    void makeMRU(GrPlot* plot);

    GrGpu*         fGpu;
    GrPixelConfig  fPixelConfig;
    GrTextureFlags fFlags;
    GrTexture*     fTexture;
    SkISize        fBackingTextureSize;
    int            fNumPlotsX;
    int            fNumPlotsY;
    bool           fBatchUploads;

    
    GrPlot*       fPlotArray;
    
    GrPlotList    fPlotList;
};

#endif
