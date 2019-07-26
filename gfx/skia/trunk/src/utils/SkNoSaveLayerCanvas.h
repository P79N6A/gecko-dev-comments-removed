






#ifndef SkNoSaveLayerCanvas_DEFINED
#define SkNoSaveLayerCanvas_DEFINED

#include "SkCanvas.h"
#include "SkRRect.h"




class SkNoSaveLayerCanvas : public SkCanvas {
public:
    SkNoSaveLayerCanvas(SkBaseDevice* device) : INHERITED(device) {}

    
    virtual int saveLayer(const SkRect* bounds,
                          const SkPaint* paint,
                          SaveFlags flags) SK_OVERRIDE {

        
        
        int count = this->INHERITED::save(flags);
        if (NULL != bounds) {
            this->INHERITED::clipRectBounds(bounds, flags, NULL);
        }
        return count;
    }

    
    virtual bool clipRect(const SkRect& rect,
                          SkRegion::Op op,
                          bool doAA) SK_OVERRIDE {
        return this->INHERITED::clipRect(rect, op, false);
    }

    
    
    virtual bool clipPath(const SkPath& path,
                          SkRegion::Op op,
                          bool doAA) SK_OVERRIDE {
        return this->updateClipConservativelyUsingBounds(path.getBounds(), op,
                                                         path.isInverseFillType());
    }
    virtual bool clipRRect(const SkRRect& rrect,
                           SkRegion::Op op,
                           bool doAA) SK_OVERRIDE {
        return this->updateClipConservativelyUsingBounds(rrect.getBounds(), op, false);
    }

private:
    typedef SkCanvas INHERITED;
};

#endif 
