









#ifndef SkGpuCanvas_DEFINED
#define SkGpuCanvas_DEFINED

#include "SkCanvas.h"

class GrContext;
class GrRenderTarget;





class SkGpuCanvas : public SkCanvas {
public:
    







    explicit SkGpuCanvas(GrContext*, GrRenderTarget*);
    virtual ~SkGpuCanvas();

    



    virtual bool getViewport(SkIPoint* size) const;

#if 0
    virtual int saveLayer(const SkRect* bounds, const SkPaint* paint,
                          SaveFlags flags = kARGB_ClipLayer_SaveFlag) {
        return this->save(flags);
    }
#endif

private:
    GrContext* fContext;

    typedef SkCanvas INHERITED;
};

#endif


