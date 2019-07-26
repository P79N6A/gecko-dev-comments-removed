






#ifndef SkNoSaveLayerCanvas_DEFINED
#define SkNoSaveLayerCanvas_DEFINED

#include "SkCanvas.h"
#include "SkRRect.h"




class SK_API SkNoSaveLayerCanvas : public SkCanvas {
public:
    SkNoSaveLayerCanvas(SkBaseDevice* device) : INHERITED(device) {}

protected:
    virtual SaveLayerStrategy willSaveLayer(const SkRect* bounds, const SkPaint* paint,
                                            SaveFlags flags) SK_OVERRIDE {
        this->INHERITED::willSaveLayer(bounds, paint, flags);
        return kNoLayer_SaveLayerStrategy;
    }

    
    virtual void onClipRect(const SkRect& rect, SkRegion::Op op, ClipEdgeStyle) SK_OVERRIDE {
        this->INHERITED::onClipRect(rect, op, kHard_ClipEdgeStyle);
    }

    
    
    virtual void onClipPath(const SkPath& path, SkRegion::Op op, ClipEdgeStyle) SK_OVERRIDE {
        this->updateClipConservativelyUsingBounds(path.getBounds(), op,
                                                  path.isInverseFillType());
    }
    virtual void onClipRRect(const SkRRect& rrect, SkRegion::Op op, ClipEdgeStyle) SK_OVERRIDE {
        this->updateClipConservativelyUsingBounds(rrect.getBounds(), op, false);
    }

private:
    typedef SkCanvas INHERITED;
};

#endif 
