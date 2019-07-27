






#ifndef SkDrawProcs_DEFINED
#define SkDrawProcs_DEFINED

#include "SkBlitter.h"
#include "SkDraw.h"
#include "SkGlyph.h"

class SkAAClip;
class SkBlitter;

struct SkDraw1Glyph {
    const SkDraw* fDraw;
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

class SkTextAlignProc {
public:
    SkTextAlignProc(SkPaint::Align align)
        : fAlign(align) {
    }

    
    
    
    void operator()(const SkPoint& loc, const SkGlyph& glyph, SkIPoint* dst) {
        if (SkPaint::kLeft_Align == fAlign) {
            dst->set(SkScalarToFixed(loc.fX), SkScalarToFixed(loc.fY));
        } else if (SkPaint::kCenter_Align == fAlign) {
            dst->set(SkScalarToFixed(loc.fX) - (glyph.fAdvanceX >> 1),
                     SkScalarToFixed(loc.fY) - (glyph.fAdvanceY >> 1));
        } else {
            SkASSERT(SkPaint::kRight_Align == fAlign);
            dst->set(SkScalarToFixed(loc.fX) - glyph.fAdvanceX,
                     SkScalarToFixed(loc.fY) - glyph.fAdvanceY);
        }
    }
private:
    const SkPaint::Align fAlign;
};

class SkTextAlignProcScalar {
public:
    SkTextAlignProcScalar(SkPaint::Align align)
        : fAlign(align) {
    }

    
    
    void operator()(const SkPoint& loc, const SkGlyph& glyph, SkPoint* dst) {
        if (SkPaint::kLeft_Align == fAlign) {
            dst->set(loc.fX, loc.fY);
        } else if (SkPaint::kCenter_Align == fAlign) {
            dst->set(loc.fX - SkFixedToScalar(glyph.fAdvanceX >> 1),
                     loc.fY - SkFixedToScalar(glyph.fAdvanceY >> 1));
        } else {
            SkASSERT(SkPaint::kRight_Align == fAlign);
            dst->set(loc.fX - SkFixedToScalar(glyph.fAdvanceX),
                     loc.fY - SkFixedToScalar(glyph.fAdvanceY));
        }
    }
private:
    const SkPaint::Align fAlign;
};

#endif
