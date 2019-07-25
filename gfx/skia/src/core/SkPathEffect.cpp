








#include "SkPathEffect.h"
#include "SkPath.h"
#include "SkBuffer.h"

void SkPathEffect::computeFastBounds(SkRect* dst, const SkRect& src) {
    *dst = src;
}



SkPairPathEffect::SkPairPathEffect(SkPathEffect* pe0, SkPathEffect* pe1)
        : fPE0(pe0), fPE1(pe1) {
    SkASSERT(pe0);
    SkASSERT(pe1);
    fPE0->ref();
    fPE1->ref();
}

SkPairPathEffect::~SkPairPathEffect() {
    SkSafeUnref(fPE0);
    SkSafeUnref(fPE1);
}




void SkPairPathEffect::flatten(SkFlattenableWriteBuffer& buffer) const {
    this->INHERITED::flatten(buffer);
    buffer.writeFlattenable(fPE0);
    buffer.writeFlattenable(fPE1);
}

SkPairPathEffect::SkPairPathEffect(SkFlattenableReadBuffer& buffer) {
    fPE0 = (SkPathEffect*)buffer.readFlattenable();
    fPE1 = (SkPathEffect*)buffer.readFlattenable();
    
}



bool SkComposePathEffect::filterPath(SkPath* dst, const SkPath& src,
                                     SkScalar* width) {
    
    if (!fPE0 || !fPE1) {
        return false;
    }

    SkPath          tmp;
    const SkPath*   ptr = &src;

    if (fPE1->filterPath(&tmp, src, width)) {
        ptr = &tmp;
    }
    return fPE0->filterPath(dst, *ptr, width);
}



bool SkSumPathEffect::filterPath(SkPath* dst, const SkPath& src,
                                 SkScalar* width) {
    
    return  fPE0->filterPath(dst, src, width) | fPE1->filterPath(dst, src, width);
}



#include "SkStroke.h"








class SkStrokePathEffect : public SkPathEffect {
public:
    SkStrokePathEffect(const SkPaint&);
    SkStrokePathEffect(SkScalar width, SkPaint::Style, SkPaint::Join,
                       SkPaint::Cap, SkScalar miterLimit = -1);
    
    
    virtual bool filterPath(SkPath* dst, const SkPath& src, SkScalar* width);
    
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkStrokePathEffect)
    
protected:
    SkStrokePathEffect(SkFlattenableReadBuffer&);
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;
    
private:
    SkScalar    fWidth, fMiter;
    uint8_t     fStyle, fJoin, fCap;
    
    typedef SkPathEffect INHERITED;
    
    
    SkStrokePathEffect(const SkStrokePathEffect&);
    SkStrokePathEffect& operator=(const SkStrokePathEffect&);
};

SkStrokePathEffect::SkStrokePathEffect(const SkPaint& paint)
    : fWidth(paint.getStrokeWidth()), fMiter(paint.getStrokeMiter()),
      fStyle(SkToU8(paint.getStyle())), fJoin(SkToU8(paint.getStrokeJoin())),
      fCap(SkToU8(paint.getStrokeCap())) {
}

SkStrokePathEffect::SkStrokePathEffect(SkScalar width, SkPaint::Style style,
                           SkPaint::Join join, SkPaint::Cap cap, SkScalar miter)
        : fWidth(width), fMiter(miter), fStyle(SkToU8(style)),
          fJoin(SkToU8(join)), fCap(SkToU8(cap)) {
    if (miter < 0) {  
        fMiter = SkIntToScalar(4);
    }
}

bool SkStrokePathEffect::filterPath(SkPath* dst, const SkPath& src,
                                    SkScalar* width) {
    if (fWidth < 0 || fStyle == SkPaint::kFill_Style) {
        return false;
    }

    if (fStyle == SkPaint::kStroke_Style && fWidth == 0) {  
        *width = 0;
        return true;
    }

    SkStroke    stroke;

    stroke.setWidth(fWidth);
    stroke.setMiterLimit(fMiter);
    stroke.setJoin((SkPaint::Join)fJoin);
    stroke.setCap((SkPaint::Cap)fCap);
    stroke.setDoFill(fStyle == SkPaint::kStrokeAndFill_Style);

    stroke.strokePath(src, dst);
    return true;
}

void SkStrokePathEffect::flatten(SkFlattenableWriteBuffer& buffer) const {
    this->INHERITED::flatten(buffer);
    buffer.writeScalar(fWidth);
    buffer.writeScalar(fMiter);
    buffer.write8(fStyle);
    buffer.write8(fJoin);
    buffer.write8(fCap);
}

SkStrokePathEffect::SkStrokePathEffect(SkFlattenableReadBuffer& buffer) {
    fWidth = buffer.readScalar();
    fMiter = buffer.readScalar();
    fStyle = buffer.readU8();
    fJoin = buffer.readU8();
    fCap = buffer.readU8();
}



SK_DEFINE_FLATTENABLE_REGISTRAR(SkComposePathEffect)

SK_DEFINE_FLATTENABLE_REGISTRAR(SkSumPathEffect)

