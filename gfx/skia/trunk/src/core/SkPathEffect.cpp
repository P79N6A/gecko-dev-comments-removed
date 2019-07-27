







#include "SkPathEffect.h"
#include "SkPath.h"
#include "SkReadBuffer.h"
#include "SkWriteBuffer.h"



void SkPathEffect::computeFastBounds(SkRect* dst, const SkRect& src) const {
    *dst = src;
}

bool SkPathEffect::asPoints(PointData* results, const SkPath& src,
                    const SkStrokeRec&, const SkMatrix&, const SkRect*) const {
    return false;
}

SkPathEffect::DashType SkPathEffect::asADash(DashInfo* info) const {
    return kNone_DashType;
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




void SkPairPathEffect::flatten(SkWriteBuffer& buffer) const {
    this->INHERITED::flatten(buffer);
    buffer.writeFlattenable(fPE0);
    buffer.writeFlattenable(fPE1);
}

SkPairPathEffect::SkPairPathEffect(SkReadBuffer& buffer) {
    fPE0 = buffer.readPathEffect();
    fPE1 = buffer.readPathEffect();
    
}



bool SkComposePathEffect::filterPath(SkPath* dst, const SkPath& src,
                             SkStrokeRec* rec, const SkRect* cullRect) const {
    
    if (!fPE0 || !fPE1) {
        return false;
    }

    SkPath          tmp;
    const SkPath*   ptr = &src;

    if (fPE1->filterPath(&tmp, src, rec, cullRect)) {
        ptr = &tmp;
    }
    return fPE0->filterPath(dst, *ptr, rec, cullRect);
}



bool SkSumPathEffect::filterPath(SkPath* dst, const SkPath& src,
                             SkStrokeRec* rec, const SkRect* cullRect) const {
    
    return fPE0->filterPath(dst, src, rec, cullRect) |
           fPE1->filterPath(dst, src, rec, cullRect);
}
