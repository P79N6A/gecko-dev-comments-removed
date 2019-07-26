








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

    private:
        SkDeque* fLayers;
    };

#ifdef SK_SUPPORT_LEGACY_LAYERRASTERIZER_API
    void addLayer(const SkPaint& paint) {
        this->addLayer(paint, 0, 0);
    }

    




    void addLayer(const SkPaint& paint, SkScalar dx, SkScalar dy);
#endif

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkLayerRasterizer)

protected:
    SkLayerRasterizer(SkDeque* layers);
    SkLayerRasterizer(SkReadBuffer&);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

    
    virtual bool onRasterize(const SkPath& path, const SkMatrix& matrix,
                             const SkIRect* clipBounds,
                             SkMask* mask, SkMask::CreateMode mode) const;

#ifdef SK_SUPPORT_LEGACY_LAYERRASTERIZER_API
public:
#endif
    SkLayerRasterizer();

private:
#ifdef SK_SUPPORT_LEGACY_LAYERRASTERIZER_API
    SkDeque* fLayers;
#else
    const SkDeque* const fLayers;
#endif

    static SkDeque* ReadLayers(SkReadBuffer& buffer);

    typedef SkRasterizer INHERITED;
};

#endif
