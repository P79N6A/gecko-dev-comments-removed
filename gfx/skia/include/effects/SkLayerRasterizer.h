








#ifndef SkLayerRasterizer_DEFINED
#define SkLayerRasterizer_DEFINED

#include "SkRasterizer.h"
#include "SkDeque.h"
#include "SkScalar.h"

class SkPaint;

class SkLayerRasterizer : public SkRasterizer {
public:
            SkLayerRasterizer();
    virtual ~SkLayerRasterizer();
    
    void addLayer(const SkPaint& paint) {
        this->addLayer(paint, 0, 0);
    }

	




    void addLayer(const SkPaint& paint, SkScalar dx, SkScalar dy);

    
    virtual Factory getFactory();
    virtual void    flatten(SkFlattenableWriteBuffer&);

    static SkFlattenable* CreateProc(SkFlattenableReadBuffer&);

protected:
    SkLayerRasterizer(SkFlattenableReadBuffer&);

    
    virtual bool onRasterize(const SkPath& path, const SkMatrix& matrix,
                             const SkIRect* clipBounds,
                             SkMask* mask, SkMask::CreateMode mode);

private:
    SkDeque fLayers;

    typedef SkRasterizer INHERITED;
};

#endif
