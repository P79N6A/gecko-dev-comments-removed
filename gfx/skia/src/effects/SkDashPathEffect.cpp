








#include "SkDashPathEffect.h"
#include "SkFlattenableBuffers.h"
#include "SkPathMeasure.h"

static inline int is_even(int x) {
    return (~x) << 31;
}

static SkScalar FindFirstInterval(const SkScalar intervals[], SkScalar phase,
                                  int32_t* index, int count) {
    for (int i = 0; i < count; ++i) {
        if (phase > intervals[i]) {
            phase -= intervals[i];
        } else {
            *index = i;
            return intervals[i] - phase;
        }
    }
    
    
    
    
    *index = 0;
    return intervals[0];
}

SkDashPathEffect::SkDashPathEffect(const SkScalar intervals[], int count,
                                   SkScalar phase, bool scaleToFit)
        : fScaleToFit(scaleToFit) {
    SkASSERT(intervals);
    SkASSERT(count > 1 && SkAlign2(count) == count);

    fIntervals = (SkScalar*)sk_malloc_throw(sizeof(SkScalar) * count);
    fCount = count;

    SkScalar len = 0;
    for (int i = 0; i < count; i++) {
        SkASSERT(intervals[i] >= 0);
        fIntervals[i] = intervals[i];
        len += intervals[i];
    }
    fIntervalLength = len;

    
    if ((len > 0) && SkScalarIsFinite(phase) && SkScalarIsFinite(len)) {

        
        
        if (phase < 0) {
            phase = -phase;
            if (phase > len) {
                phase = SkScalarMod(phase, len);
            }
            phase = len - phase;

            
            
            
            SkASSERT(phase <= len);
            if (phase == len) {
                phase = 0;
            }
        } else if (phase >= len) {
            phase = SkScalarMod(phase, len);
        }
        SkASSERT(phase >= 0 && phase < len);

        fInitialDashLength = FindFirstInterval(intervals, phase,
                                               &fInitialDashIndex, count);

        SkASSERT(fInitialDashLength >= 0);
        SkASSERT(fInitialDashIndex >= 0 && fInitialDashIndex < fCount);
    } else {
        fInitialDashLength = -1;    
    }
}

SkDashPathEffect::~SkDashPathEffect() {
    sk_free(fIntervals);
}

class SpecialLineRec {
public:
    bool init(const SkPath& src, SkPath* dst, SkStrokeRec* rec,
              SkScalar pathLength,
              int intervalCount, SkScalar intervalLength) {
        if (rec->isHairlineStyle() || !src.isLine(fPts)) {
            return false;
        }

        
        if (SkPaint::kButt_Cap != rec->getCap()) {
            return false;
        }

        fTangent = fPts[1] - fPts[0];
        if (fTangent.isZero()) {
            return false;
        }

        fPathLength = pathLength;
        fTangent.scale(SkScalarInvert(pathLength));
        fTangent.rotateCCW(&fNormal);
        fNormal.scale(SkScalarHalf(rec->getWidth()));

        
        
        

        SkScalar ptCount = SkScalarMulDiv(pathLength,
                                          SkIntToScalar(intervalCount),
                                          intervalLength);
        int n = SkScalarCeilToInt(ptCount) << 2;
        dst->incReserve(n);

        
        rec->setFillStyle();
        return true;
    }

    void addSegment(SkScalar d0, SkScalar d1, SkPath* path) const {
        SkASSERT(d0 < fPathLength);
        
        if (d1 > fPathLength) {
            d1 = fPathLength;
        }

        SkScalar x0 = fPts[0].fX + SkScalarMul(fTangent.fX, d0);
        SkScalar x1 = fPts[0].fX + SkScalarMul(fTangent.fX, d1);
        SkScalar y0 = fPts[0].fY + SkScalarMul(fTangent.fY, d0);
        SkScalar y1 = fPts[0].fY + SkScalarMul(fTangent.fY, d1);

        SkPoint pts[4];
        pts[0].set(x0 + fNormal.fX, y0 + fNormal.fY);   
        pts[1].set(x1 + fNormal.fX, y1 + fNormal.fY);   
        pts[2].set(x1 - fNormal.fX, y1 - fNormal.fY);   
        pts[3].set(x0 - fNormal.fX, y0 - fNormal.fY);   

        path->addPoly(pts, SK_ARRAY_COUNT(pts), false);
    }

private:
    SkPoint fPts[2];
    SkVector fTangent;
    SkVector fNormal;
    SkScalar fPathLength;
};

bool SkDashPathEffect::filterPath(SkPath* dst, const SkPath& src,
                                  SkStrokeRec* rec) {
    
    if (rec->isFillStyle() || fInitialDashLength < 0) {
        return false;
    }

    SkPathMeasure   meas(src, false);
    const SkScalar* intervals = fIntervals;

    SpecialLineRec lineRec;
    const bool specialLine = lineRec.init(src, dst, rec, meas.getLength(),
                                          fCount >> 1, fIntervalLength);

    do {
        bool        skipFirstSegment = meas.isClosed();
        bool        addedSegment = false;
        SkScalar    length = meas.getLength();
        int         index = fInitialDashIndex;
        SkScalar    scale = SK_Scalar1;

        if (fScaleToFit) {
            if (fIntervalLength >= length) {
                scale = SkScalarDiv(length, fIntervalLength);
            } else {
                SkScalar div = SkScalarDiv(length, fIntervalLength);
                int n = SkScalarFloor(div);
                scale = SkScalarDiv(length, n * fIntervalLength);
            }
        }

        SkScalar    distance = 0;
        SkScalar    dlen = SkScalarMul(fInitialDashLength, scale);

        while (distance < length) {
            SkASSERT(dlen >= 0);
            addedSegment = false;
            if (is_even(index) && dlen > 0 && !skipFirstSegment) {
                addedSegment = true;

                if (specialLine) {
                    lineRec.addSegment(distance, distance + dlen, dst);
                } else {
                    meas.getSegment(distance, distance + dlen, dst, true);
                }
            }
            distance += dlen;

            
            skipFirstSegment = false;

            
            index += 1;
            SkASSERT(index <= fCount);
            if (index == fCount) {
                index = 0;
            }

            
            dlen = SkScalarMul(intervals[index], scale);
        }

        
        if (meas.isClosed() && is_even(fInitialDashIndex) &&
                fInitialDashLength > 0) {
            meas.getSegment(0, SkScalarMul(fInitialDashLength, scale), dst, !addedSegment);
        }
    } while (meas.nextContour());

    return true;
}

SkFlattenable::Factory SkDashPathEffect::getFactory() {
    return fInitialDashLength < 0 ? NULL : CreateProc;
}

void SkDashPathEffect::flatten(SkFlattenableWriteBuffer& buffer) const {
    SkASSERT(fInitialDashLength >= 0);

    this->INHERITED::flatten(buffer);
    buffer.writeInt(fInitialDashIndex);
    buffer.writeScalar(fInitialDashLength);
    buffer.writeScalar(fIntervalLength);
    buffer.writeBool(fScaleToFit);
    buffer.writeScalarArray(fIntervals, fCount);
}

SkFlattenable* SkDashPathEffect::CreateProc(SkFlattenableReadBuffer& buffer) {
    return SkNEW_ARGS(SkDashPathEffect, (buffer));
}

SkDashPathEffect::SkDashPathEffect(SkFlattenableReadBuffer& buffer) : INHERITED(buffer) {
    fInitialDashIndex = buffer.readInt();
    fInitialDashLength = buffer.readScalar();
    fIntervalLength = buffer.readScalar();
    fScaleToFit = buffer.readBool();

    fCount = buffer.getArrayCount();
    fIntervals = (SkScalar*)sk_malloc_throw(sizeof(SkScalar) * fCount);
    buffer.readScalarArray(fIntervals);
}



SK_DEFINE_FLATTENABLE_REGISTRAR(SkDashPathEffect)
