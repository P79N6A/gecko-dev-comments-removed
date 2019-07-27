





#include "SkOpContour.h"
#include "SkPath.h"

#ifdef SK_DEBUG
#include "SkPathOpsPoint.h"
#endif

class SkIntersectionHelper {
public:
    enum SegmentType {
        kHorizontalLine_Segment = -1,
        kVerticalLine_Segment = 0,
        kLine_Segment = SkPath::kLine_Verb,
        kQuad_Segment = SkPath::kQuad_Verb,
        kCubic_Segment = SkPath::kCubic_Verb,
    };

    bool addCoincident(SkIntersectionHelper& other, const SkIntersections& ts, bool swap) {
        return fContour->addCoincident(fIndex, other.fContour, other.fIndex, ts, swap);
    }

    
    
    void addOtherT(int index, double otherT, int otherIndex) {
        fContour->addOtherT(fIndex, index, otherT, otherIndex);
    }

    bool addPartialCoincident(SkIntersectionHelper& other, const SkIntersections& ts, int index,
            bool swap) {
        return fContour->addPartialCoincident(fIndex, other.fContour, other.fIndex, ts, index,
                swap);
    }

    
    
    
    
    
    int addT(const SkIntersectionHelper& other, const SkPoint& pt, double newT) {
        return fContour->addT(fIndex, other.fContour, other.fIndex, pt, newT);
    }

    int addSelfT(const SkPoint& pt, double newT) {
        return fContour->addSelfT(fIndex, pt, newT);
    }

    bool advance() {
        return ++fIndex < fLast;
    }

    void alignTPt(SkIntersectionHelper& other, bool swap, int index,
            SkIntersections* ts, SkPoint* point) {
        fContour->alignTPt(fIndex, other.fContour, other.fIndex, swap, index, ts, point);
    }

    SkScalar bottom() const {
        return bounds().fBottom;
    }

    const SkPathOpsBounds& bounds() const {
        return fContour->segments()[fIndex].bounds();
    }

    void init(SkOpContour* contour) {
        fContour = contour;
        fIndex = 0;
        fLast = contour->segments().count();
    }

    bool isAdjacent(const SkIntersectionHelper& next) {
        return fContour == next.fContour && fIndex + 1 == next.fIndex;
    }

    bool isFirstLast(const SkIntersectionHelper& next) {
        return fContour == next.fContour && fIndex == 0
                && next.fIndex == fLast - 1;
    }

    bool isPartial(double t1, double t2, const SkDPoint& pt1, const SkDPoint& pt2) const {
        const SkOpSegment& segment = fContour->segments()[fIndex];
        double mid = (t1 + t2) / 2;
        SkDPoint midPtByT = segment.dPtAtT(mid);
        SkDPoint midPtByAvg = SkDPoint::Mid(pt1, pt2);
        return midPtByT.approximatelyPEqual(midPtByAvg);
    }

    SkScalar left() const {
        return bounds().fLeft;
    }

    const SkPoint* pts() const {
        return fContour->segments()[fIndex].pts();
    }

    SkScalar right() const {
        return bounds().fRight;
    }

    SegmentType segmentType() const {
        const SkOpSegment& segment = fContour->segments()[fIndex];
        SegmentType type = (SegmentType) segment.verb();
        if (type != kLine_Segment) {
            return type;
        }
        if (segment.isHorizontal()) {
            return kHorizontalLine_Segment;
        }
        if (segment.isVertical()) {
            return kVerticalLine_Segment;
        }
        return kLine_Segment;
    }

    bool startAfter(const SkIntersectionHelper& after) {
        fIndex = after.fIndex;
        return advance();
    }

    SkScalar top() const {
        return bounds().fTop;
    }

    SkPath::Verb verb() const {
        return fContour->segments()[fIndex].verb();
    }

    SkScalar x() const {
        return bounds().fLeft;
    }

    bool xFlipped() const {
        return x() != pts()[0].fX;
    }

    SkScalar y() const {
        return bounds().fTop;
    }

    bool yFlipped() const {
        return y() != pts()[0].fY;
    }

private:
    
    void dump() const;

    SkOpContour* fContour;
    int fIndex;
    int fLast;
};
