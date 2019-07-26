






#ifndef SkDrawProcs_DEFINED
#define SkDrawProcs_DEFINED

#include "SkBlitter.h"
#include "SkDraw.h"

class SkAAClip;
class SkBlitter;

struct SkDraw1Glyph {
    const SkDraw* fDraw;
    SkBounder* fBounder;
    const SkRegion* fClip;
    const SkAAClip* fAAClip;
    SkBlitter* fBlitter;
    SkGlyphCache* fCache;
    const SkPaint* fPaint;
    SkIRect fClipBounds;
    
    SkFixed fHalfSampleX;
    
    SkFixed fHalfSampleY;

    






    typedef void (*Proc)(const SkDraw1Glyph&, SkFixed x, SkFixed y, const SkGlyph&);

    Proc init(const SkDraw* draw, SkBlitter* blitter, SkGlyphCache* cache,
              const SkPaint&);

    
    
    
    void blitMask(const SkMask& mask, const SkIRect& clip) const {
        if (SkMask::kARGB32_Format == mask.fFormat) {
            this->blitMaskAsSprite(mask);
        } else {
            fBlitter->blitMask(mask, clip);
        }
    }

    
    void blitMaskAsSprite(const SkMask& mask) const;
};

struct SkDrawProcs {
    SkDraw1Glyph::Proc  fD1GProc;
};

bool SkDrawTreatAAStrokeAsHairline(SkScalar strokeWidth, const SkMatrix&,
                                   SkScalar* coverage);







inline bool SkDrawTreatAsHairline(const SkPaint& paint, const SkMatrix& matrix,
                                  SkScalar* coverage) {
    if (SkPaint::kStroke_Style != paint.getStyle()) {
        return false;
    }

    SkScalar strokeWidth = paint.getStrokeWidth();
    if (0 == strokeWidth) {
        *coverage = SK_Scalar1;
        return true;
    }

    if (!paint.isAntiAlias()) {
        return false;
    }

    return SkDrawTreatAAStrokeAsHairline(strokeWidth, matrix, coverage);
}

#endif
