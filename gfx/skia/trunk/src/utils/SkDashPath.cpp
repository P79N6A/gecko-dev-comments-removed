






#include "SkDashPathPriv.h"
#include "SkPathMeasure.h"

static inline int is_even(int x) {
    return (~x) << 31;
}

static SkScalar find_first_interval(const SkScalar intervals[], SkScalar phase,
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

void SkDashPath::CalcDashParameters(SkScalar phase, const SkScalar intervals[], int32_t count,
                                    SkScalar* initialDashLength, int32_t* initialDashIndex,
                                    SkScalar* intervalLength, SkScalar* adjustedPhase) {
    SkScalar len = 0;
    for (int i = 0; i < count; i++) {
        len += intervals[i];
    }
    *intervalLength = len;

    
    if ((len > 0) && SkScalarIsFinite(phase) && SkScalarIsFinite(len)) {

        
        
        if (adjustedPhase) {
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
            *adjustedPhase = phase;
        }
        SkASSERT(phase >= 0 && phase < len);

        *initialDashLength = find_first_interval(intervals, phase,
                                                initialDashIndex, count);

        SkASSERT(*initialDashLength >= 0);
        SkASSERT(*initialDashIndex >= 0 && *initialDashIndex < count);
    } else {
        *initialDashLength = -1;    
    }
}

static void outset_for_stroke(SkRect* rect, const SkStrokeRec& rec) {
    SkScalar radius = SkScalarHalf(rec.getWidth());
    if (0 == radius) {
        radius = SK_Scalar1;    
    }
    if (SkPaint::kMiter_Join == rec.getJoin()) {
        radius = SkScalarMul(radius, rec.getMiter());
    }
    rect->outset(radius, radius);
}



static bool cull_path(const SkPath& srcPath, const SkStrokeRec& rec,
                      const SkRect* cullRect, SkScalar intervalLength,
                      SkPath* dstPath) {
    if (NULL == cullRect) {
        return false;
    }

    SkPoint pts[2];
    if (!srcPath.isLine(pts)) {
        return false;
    }

    SkRect bounds = *cullRect;
    outset_for_stroke(&bounds, rec);

    SkScalar dx = pts[1].x() - pts[0].x();
    SkScalar dy = pts[1].y() - pts[0].y();

    
    if (dy) {
        return false;
    }

    SkScalar minX = pts[0].fX;
    SkScalar maxX = pts[1].fX;

    if (maxX < bounds.fLeft || minX > bounds.fRight) {
        return false;
    }

    if (dx < 0) {
        SkTSwap(minX, maxX);
    }

    
    
    

    if (minX < bounds.fLeft) {
        minX = bounds.fLeft - SkScalarMod(bounds.fLeft - minX,
                                          intervalLength);
    }
    if (maxX > bounds.fRight) {
        maxX = bounds.fRight + SkScalarMod(maxX - bounds.fRight,
                                           intervalLength);
    }

    SkASSERT(maxX >= minX);
    if (dx < 0) {
        SkTSwap(minX, maxX);
    }
    pts[0].fX = minX;
    pts[1].fX = maxX;

    dstPath->moveTo(pts[0]);
    dstPath->lineTo(pts[1]);
    return true;
}

class SpecialLineRec {
public:
    bool init(const SkPath& src, SkPath* dst, SkStrokeRec* rec,
              int intervalCount, SkScalar intervalLength) {
        if (rec->isHairlineStyle() || !src.isLine(fPts)) {
            return false;
        }

        
        if (SkPaint::kButt_Cap != rec->getCap()) {
            return false;
        }

        SkScalar pathLength = SkPoint::Distance(fPts[0], fPts[1]);

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


bool SkDashPath::FilterDashPath(SkPath* dst, const SkPath& src, SkStrokeRec* rec,
                                const SkRect* cullRect, const SkScalar aIntervals[],
                                int32_t count, SkScalar initialDashLength, int32_t initialDashIndex,
                                SkScalar intervalLength) {

    
    if (rec->isFillStyle() || initialDashLength < 0) {
        return false;
    }

    const SkScalar* intervals = aIntervals;
    SkScalar        dashCount = 0;
    int             segCount = 0;

    SkPath cullPathStorage;
    const SkPath* srcPtr = &src;
    if (cull_path(src, *rec, cullRect, intervalLength, &cullPathStorage)) {
        srcPtr = &cullPathStorage;
    }

    SpecialLineRec lineRec;
    bool specialLine = lineRec.init(*srcPtr, dst, rec, count >> 1, intervalLength);

    SkPathMeasure   meas(*srcPtr, false);

    do {
        bool        skipFirstSegment = meas.isClosed();
        bool        addedSegment = false;
        SkScalar    length = meas.getLength();
        int         index = initialDashIndex;

        
        
        
        
        
        
        
        
        static const SkScalar kMaxDashCount = 1000000;
        dashCount += length * (count >> 1) / intervalLength;
        if (dashCount > kMaxDashCount) {
            dst->reset();
            return false;
        }

        
        
        double  distance = 0;
        double  dlen = initialDashLength;

        while (distance < length) {
            SkASSERT(dlen >= 0);
            addedSegment = false;
            if (is_even(index) && dlen > 0 && !skipFirstSegment) {
                addedSegment = true;
                ++segCount;

                if (specialLine) {
                    lineRec.addSegment(SkDoubleToScalar(distance),
                                       SkDoubleToScalar(distance + dlen),
                                       dst);
                } else {
                    meas.getSegment(SkDoubleToScalar(distance),
                                    SkDoubleToScalar(distance + dlen),
                                    dst, true);
                }
            }
            distance += dlen;

            
            skipFirstSegment = false;

            
            index += 1;
            SkASSERT(index <= count);
            if (index == count) {
                index = 0;
            }

            
            dlen = intervals[index];
        }

        
        if (meas.isClosed() && is_even(initialDashIndex) &&
            initialDashLength > 0) {
            meas.getSegment(0, initialDashLength, dst, !addedSegment);
            ++segCount;
        }
    } while (meas.nextContour());

    if (segCount > 1) {
        dst->setConvexity(SkPath::kConcave_Convexity);
    }

    return true;
}

bool SkDashPath::FilterDashPath(SkPath* dst, const SkPath& src, SkStrokeRec* rec,
                                const SkRect* cullRect, const SkPathEffect::DashInfo& info) {
    SkScalar initialDashLength = 0;
    int32_t initialDashIndex = 0;
    SkScalar intervalLength = 0;
    CalcDashParameters(info.fPhase, info.fIntervals, info.fCount,
                       &initialDashLength, &initialDashIndex, &intervalLength);
    return FilterDashPath(dst, src, rec, cullRect, info.fIntervals, info.fCount, initialDashLength,
                          initialDashIndex, intervalLength);
}
