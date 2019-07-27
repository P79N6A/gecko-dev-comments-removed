








#ifndef SkLayerRasterizer_DEFINED
#define SkLayerRasterizer_DEFINED

#include "SkRasterizer.h"
#include "SkDeque.h"
#include "SkScalar.h"

class SkPaint;

class SK_API SkLayerRasterizer : public SkRasterizer {
public:
    virtual ~SkLayerRasterizer();

    class SK_API Builder {
    public:
        Builder();
        ~Builder();

        void addLayer(const SkPaint& paint) {
            this->addLayer(paint, 0, 0);
        }

        





        void addLayer(const SkPaint& paint, SkScalar dx, SkScalar dy);

        






        SkLayerRasterizer* detachRasterizer();

        











        SkLayerRasterizer* snapshotRasterizer() const;

    private:
        SkDeque* fLayers;
    };

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkLayerRasterizer)

protected:
    SkLayerRasterizer();
    SkLayerRasterizer(SkDeque* layers);
    SkLayerRasterizer(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

    
    virtual bool onRasterize(const SkPath& path, const SkMatrix& matrix,
                             const SkIRect* clipBounds,
                             SkMask* mask, SkMask::CreateMode mode) const;

private:
    const SkDeque* const fLayers;

    static SkDeque* ReadLayers(SkReadBuffer& buffer);

    friend class LayerRasterizerTester;

    typedef SkRasterizer INHERITED;
};

#endif
