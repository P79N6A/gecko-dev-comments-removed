







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
    int getOffsetX() const { return fOffset.fX; }
    int getOffsetY() const { return fOffset.fY; }

    GrTexture* texture() const { return fTexture; }

    bool addSubImage(int width, int height, const void*, GrIPoint16*);

    GrDrawTarget::DrawToken drawToken() const { return fDrawToken; }
    void setDrawToken(GrDrawTarget::DrawToken draw) { fDrawToken = draw; }

private:
    GrPlot();
    ~GrPlot(); 

    
    GrDrawTarget::DrawToken fDrawToken;

    GrPlot*                 fNext;

    GrTexture*              fTexture;
    GrRectanizer*           fRects;
    GrAtlasMgr*             fAtlasMgr;
    GrIPoint16              fOffset;
    size_t                  fBytesPerPixel;

    friend class GrAtlasMgr;
};

class GrAtlasMgr {
public:
    GrAtlasMgr(GrGpu*, GrPixelConfig);
    ~GrAtlasMgr();

    
    
    GrPlot* addToAtlas(GrAtlas*, int width, int height, const void*, GrIPoint16*);

    
    bool removeUnusedPlots(GrAtlas* atlas);

    
    void deletePlotList(GrPlot* plot);

    GrTexture* getTexture() const {
        return fTexture;
    }

private:
    GrPlot* allocPlot();
    void freePlot(GrPlot* plot);

    GrGpu*        fGpu;
    GrPixelConfig fPixelConfig;
    GrTexture*    fTexture;

    
    GrPlot*       fPlots;
    
    GrPlot*       fFreePlots;
};

class GrAtlas {
public:
    GrAtlas(GrAtlasMgr* mgr) : fPlots(NULL), fAtlasMgr(mgr) { }
    ~GrAtlas() { fAtlasMgr->deletePlotList(fPlots); }

    bool isEmpty() { return NULL == fPlots; }

private:
    GrPlot*     fPlots;
    GrAtlasMgr* fAtlasMgr;

    friend class GrAtlasMgr;
};

#endif
