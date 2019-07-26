







#ifndef GrAtlas_DEFINED
#define GrAtlas_DEFINED


#include "GrPoint.h"
#include "GrTexture.h"
#include "GrDrawTarget.h"

class GrGpu;
class GrRectanizer;
class GrAtlasMgr;
class GrAtlas;











class GrPlot {
public:
    SK_DECLARE_INTERNAL_LLIST_INTERFACE(GrPlot);

    int getOffsetX() const { return fOffset.fX; }
    int getOffsetY() const { return fOffset.fY; }

    GrTexture* texture() const { return fTexture; }

    bool addSubImage(int width, int height, const void*, GrIPoint16*);

    GrDrawTarget::DrawToken drawToken() const { return fDrawToken; }
    void setDrawToken(GrDrawTarget::DrawToken draw) { fDrawToken = draw; }

    void resetRects();

private:
    GrPlot();
    ~GrPlot(); 

    
    GrDrawTarget::DrawToken fDrawToken;

    GrTexture*              fTexture;
    GrRectanizer*           fRects;
    GrAtlasMgr*             fAtlasMgr;
    GrIPoint16              fOffset;
    size_t                  fBytesPerPixel;

    friend class GrAtlasMgr;
};

typedef SkTInternalLList<GrPlot> GrPlotList;

class GrAtlasMgr {
public:
    GrAtlasMgr(GrGpu*, GrPixelConfig);
    ~GrAtlasMgr();

    
    
    GrPlot* addToAtlas(GrAtlas*, int width, int height, const void*, GrIPoint16*);

    
    bool removePlot(GrAtlas* atlas, const GrPlot* plot);

    
    
    GrPlot* getUnusedPlot();

    GrTexture* getTexture() const {
        return fTexture;
    }

private:
    void moveToHead(GrPlot* plot);

    GrGpu*        fGpu;
    GrPixelConfig fPixelConfig;
    GrTexture*    fTexture;

    
    GrPlot*       fPlotArray;
    
    GrPlotList    fPlotList;
};

class GrAtlas {
public:
    GrAtlas() { }
    ~GrAtlas() { }

    bool isEmpty() { return 0 == fPlots.count(); }

    SkISize getSize() const;

private:
    SkTDArray<GrPlot*> fPlots;

    friend class GrAtlasMgr;
};

#endif
