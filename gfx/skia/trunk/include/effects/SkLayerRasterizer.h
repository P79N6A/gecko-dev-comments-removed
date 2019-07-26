








#ifndef SkLayerRasterizer_DEFINED
#define SkLayerRasterizer_DEFINED

#include "SkRasterizer.h"
#include "SkDeque.h"
#include "SkScalar.h"

class SkPaint;

class SK_API SkLayerRasterizer : public SkRasterizer {
public:
            SkLayerRasterizer();
    virtual ~SkLayerRasterizer();

    void addLayer(const SkPaint& paint) {
        this->addLayer(paint, 0, 0);
    }

    




    void addLayer(const SkPaint& paint, SkScalar dx, SkScalar dy);

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkLayerRasterizer)

protected:
    SkLayerRasterizer(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

    
    virtual bool onRasterize(const SkPath& path, const SkMatrix& matrix,
                             const SkIRect* clipBounds,
                             SkMask* mask, SkMask::CreateMode mode) const;

private:
    SkDeque fLayers;

    typedef SkRasterizer INHERITED;
};

#endif
