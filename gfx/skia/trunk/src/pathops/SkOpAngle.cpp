





#include "SkIntersections.h"
#include "SkOpAngle.h"
#include "SkOpSegment.h"
#include "SkPathOpsCurve.h"
#include "SkTSort.h"

#if DEBUG_ANGLE
#include "SkString.h"

static const char funcName[] = "SkOpSegment::operator<";
static const int bugChar = strlen(funcName) + 1;
#endif




#if DEBUG_ANGLE
    static bool CompareResult(SkString* bugOut, const char* append, bool compare) {
        bugOut->appendf("%s", append);
        bugOut->writable_str()[bugChar] = "><"[compare];
        SkDebugf("%s\n", bugOut->c_str());
        return compare;
    }

    #define COMPARE_RESULT(append, compare) CompareResult(&bugOut, append, compare)
#else
    #define COMPARE_RESULT(append, compare) compare
#endif

bool SkOpAngle::calcSlop(double x, double y, double rx, double ry, bool* result) const{
    double absX = fabs(x);
    double absY = fabs(y);
    double length = absX < absY ? absX / 2 + absY : absX + absY / 2;
    int exponent;
    (void) frexp(length, &exponent);
    double epsilon = ldexp(FLT_EPSILON, exponent);
    SkPath::Verb verb = fSegment->verb();
    SkASSERT(verb == SkPath::kQuad_Verb || verb == SkPath::kCubic_Verb);
    
    double slop = verb == SkPath::kQuad_Verb ? 4 * epsilon : 512 * epsilon;
    double xSlop = slop;
    double ySlop = x * y < 0 ? -xSlop : xSlop; 
    double x1 = x - xSlop;
    double y1 = y + ySlop;
    double x_ry1 = x1 * ry;
    double rx_y1 = rx * y1;
    *result = x_ry1 < rx_y1;
    double x2 = x + xSlop;
    double y2 = y - ySlop;
    double x_ry2 = x2 * ry;
    double rx_y2 = rx * y2;
    bool less2 = x_ry2 < rx_y2;
    return *result == less2;
}










bool SkOpAngle::operator<(const SkOpAngle& rh) const {  
    double y = dy();
    double ry = rh.dy();
#if DEBUG_ANGLE
    SkString bugOut;
    bugOut.printf("%s _ id=%d segId=%d tStart=%1.9g tEnd=%1.9g"
        " | id=%d segId=%d tStart=%1.9g tEnd=%1.9g ", funcName,
        fID, fSegment->debugID(), fSegment->t(fStart), fSegment->t(fEnd),
        rh.fID, rh.fSegment->debugID(), rh.fSegment->t(rh.fStart), rh.fSegment->t(rh.fEnd));
#endif
    double y_ry = y * ry;
    if (y_ry < 0) {  
        return COMPARE_RESULT("1 y * ry < 0", y < 0);
    }
    
    double x = dx();
    double rx = rh.dx();
    if (x * rx < 0) {  
        if (y < 0 && ry < 0) {  
            return COMPARE_RESULT("2 x_rx < 0 && y < 0 ...", x > 0);
        }
        if (y >= 0 && ry >= 0) {  
            return COMPARE_RESULT("3 x_rx < 0 && y >= 0 ...", x < 0);
        }
        SkASSERT((y == 0) ^ (ry == 0));  
        return COMPARE_RESULT("4 x_rx < 0 && y == 0 ...", y < 0);
    }
    
    if (y_ry == 0) { 
        if (y + ry < 0) { 
            return COMPARE_RESULT("5 y_ry == 0 && y + ry < 0", y < 0);
        }
        if (y + ry > 0) { 
            return COMPARE_RESULT("6 y_ry == 0 && y + ry > 0", (x + rx > 0) ^ (y == 0));
        }
        
        SkASSERT(x * rx != 0);  
    }
    
    if (fSegment->other(fEnd) != rh.fSegment  
            && rh.fSegment->other(rh.fEnd) != fSegment
            && y != -DBL_EPSILON
            && ry != -DBL_EPSILON) {  
        SkOpAngle longer = *this;
        SkOpAngle rhLonger = rh;
        if ((longer.lengthen(rh) | rhLonger.lengthen(*this))  
                && (fUnorderable || !longer.fUnorderable)
                && (rh.fUnorderable || !rhLonger.fUnorderable)) {
#if DEBUG_ANGLE
            bugOut.prepend("  ");
#endif
            return COMPARE_RESULT("10 longer.lengthen(rh) ...", longer < rhLonger);
        }
    }
    SkPath::Verb verb = fSegment->verb();
    SkPath::Verb rVerb = rh.fSegment->verb();
    if (y_ry != 0) { 
        
        
        double x_ry = x * ry;
        double rx_y = rx * y;
        if (!fComputed && !rh.fComputed) {
            if (!SkDLine::NearRay(x, y, rx, ry) && x_ry != rx_y) {
                return COMPARE_RESULT("7 !fComputed && !rh.fComputed", x_ry < rx_y);
            }
            if (fSide2 == 0 && rh.fSide2 == 0) {
                return COMPARE_RESULT("7a !fComputed && !rh.fComputed", x_ry < rx_y);
            }
        } else {
            
            bool sloppy1 = x_ry < rx_y;
            bool sloppy2 = !sloppy1;
            if ((!fComputed || calcSlop(x, y, rx, ry, &sloppy1))
                    && (!rh.fComputed || rh.calcSlop(rx, ry, x, y, &sloppy2))
                    && sloppy1 != sloppy2) {
                return COMPARE_RESULT("8 CalcSlop(x, y ...", sloppy1);
            }
        }
    }
    if (fSide2 * rh.fSide2 == 0) {  
#if DEBUG_ANGLE
        if (fSide2 == rh.fSide2 && y_ry) {  
            SkDebugf("%s coincidence!\n", __FUNCTION__);
        }
#endif
        return COMPARE_RESULT("9a fSide2 * rh.fSide2 == 0 ...", fSide2 < rh.fSide2);
    }
    
    
    if (fSide * rh.fSide < 0 && (!approximately_zero(fSide) || !approximately_zero(rh.fSide))) {
        return COMPARE_RESULT("9b fSide * rh.fSide < 0 ...", fSide < rh.fSide);
    }
    if (fUnsortable || rh.fUnsortable) {
        
        return COMPARE_RESULT("11 fUnsortable || rh.fUnsortable", this < &rh);
    }
    if ((verb == SkPath::kLine_Verb && approximately_zero(y) && approximately_zero(x))
            || (rVerb == SkPath::kLine_Verb
            && approximately_zero(ry) && approximately_zero(rx))) {
        
        
        fUnsortable = true;
        return COMPARE_RESULT("12 verb == SkPath::kLine_Verb ...", this < &rh);
    }
    if (fSegment->isTiny(this) || rh.fSegment->isTiny(&rh)) {
        fUnsortable = true;
        return COMPARE_RESULT("13 verb == fSegment->isTiny(this) ...", this < &rh);
    }
    SkASSERT(verb >= SkPath::kQuad_Verb);
    SkASSERT(rVerb >= SkPath::kQuad_Verb);
    
    
    
    
    double len = fTangentPart.normalSquared();
    double rlen = rh.fTangentPart.normalSquared();
    SkDLine ray;
    SkIntersections i, ri;
    int roots, rroots;
    bool flip = false;
    bool useThis;
    bool leftLessThanRight = fSide > 0;
    do {
        useThis = (len < rlen) ^ flip;
        const SkDCubic& part = useThis ? fCurvePart : rh.fCurvePart;
        SkPath::Verb partVerb = useThis ? verb : rVerb;
        ray[0] = partVerb == SkPath::kCubic_Verb && part[0].approximatelyEqual(part[1]) ?
            part[2] : part[1];
        ray[1] = SkDPoint::Mid(part[0], part[SkPathOpsVerbToPoints(partVerb)]);
        SkASSERT(ray[0] != ray[1]);
        roots = (i.*CurveRay[SkPathOpsVerbToPoints(verb)])(fSegment->pts(), ray);
        rroots = (ri.*CurveRay[SkPathOpsVerbToPoints(rVerb)])(rh.fSegment->pts(), ray);
    } while ((roots == 0 || rroots == 0) && (flip ^= true));
    if (roots == 0 || rroots == 0) {
        
        
        
        fUnsortable = true;
        return COMPARE_RESULT("roots == 0 || rroots == 0", this < &rh);
    }
    SkASSERT(fSide != 0 && rh.fSide != 0);
    if (fSide * rh.fSide < 0) {
        fUnsortable = true;
        return COMPARE_RESULT("14 fSide * rh.fSide < 0", this < &rh);
    }
    SkDPoint lLoc;
    double best = SK_ScalarInfinity;
#if DEBUG_SORT
    SkDebugf("lh=%d rh=%d use-lh=%d ray={{%1.9g,%1.9g}, {%1.9g,%1.9g}} %c\n",
            fSegment->debugID(), rh.fSegment->debugID(), useThis, ray[0].fX, ray[0].fY,
            ray[1].fX, ray[1].fY, "-+"[fSide > 0]);
#endif
    for (int index = 0; index < roots; ++index) {
        SkDPoint loc = i.pt(index);
        SkDVector dxy = loc - ray[0];
        double dist = dxy.lengthSquared();
#if DEBUG_SORT
        SkDebugf("best=%1.9g dist=%1.9g loc={%1.9g,%1.9g} dxy={%1.9g,%1.9g}\n",
                best, dist, loc.fX, loc.fY, dxy.fX, dxy.fY);
#endif
        if (best > dist) {
            lLoc = loc;
            best = dist;
        }
    }
    flip = false;
    SkDPoint rLoc;
    for (int index = 0; index < rroots; ++index) {
        rLoc = ri.pt(index);
        SkDVector dxy = rLoc - ray[0];
        double dist = dxy.lengthSquared();
#if DEBUG_SORT
        SkDebugf("best=%1.9g dist=%1.9g %c=(fSide < 0) rLoc={%1.9g,%1.9g} dxy={%1.9g,%1.9g}\n",
                best, dist, "><"[fSide < 0], rLoc.fX, rLoc.fY, dxy.fX, dxy.fY);
#endif
        if (best > dist) {
            flip = true;
            break;
        }
    }
    if (flip) {
        leftLessThanRight = !leftLessThanRight;
    }
    return COMPARE_RESULT("15 leftLessThanRight", leftLessThanRight);
}

bool SkOpAngle::isHorizontal() const {
    return dy() == 0 && fSegment->verb() == SkPath::kLine_Verb;
}


bool SkOpAngle::lengthen(const SkOpAngle& opp) {
    if (fSegment->other(fEnd) == opp.fSegment) {
        return false;
    }
    
    int newEnd = fEnd;
    if (fStart < fEnd ? ++newEnd < fSegment->count() : --newEnd >= 0) {
        fEnd = newEnd;
        setSpans();
        return true;
    }
    return false;
}

void SkOpAngle::set(const SkOpSegment* segment, int start, int end) {
    fSegment = segment;
    fStart = start;
    fEnd = end;
    setSpans();
}

void SkOpAngle::setSpans() {
    fUnorderable = fSegment->isTiny(this);
    fLastMarked = NULL;
    fUnsortable = false;
    const SkPoint* pts = fSegment->pts();
    if (fSegment->verb() != SkPath::kLine_Verb) {
        fComputed = fSegment->subDivide(fStart, fEnd, &fCurvePart);
        fSegment->subDivide(fStart, fStart < fEnd ? fSegment->count() - 1 : 0, &fCurveHalf);
    }
    
    
    
    switch (fSegment->verb()) {
    case SkPath::kLine_Verb: {
        SkASSERT(fStart != fEnd);
        fCurvePart[0].set(pts[fStart > fEnd]);
        fCurvePart[1].set(pts[fStart < fEnd]);
        fComputed = false;
        
        fTangentPart.lineEndPoints(*SkTCast<SkDLine*>(&fCurvePart));
        fSide = 0;
        fSide2 = 0;
        } break;
    case SkPath::kQuad_Verb: {
        fSide2 = -fTangentHalf.quadPart(*SkTCast<SkDQuad*>(&fCurveHalf));
        SkDQuad& quad = *SkTCast<SkDQuad*>(&fCurvePart);
        fTangentPart.quadEndPoints(quad);
        fSide = -fTangentPart.pointDistance(fCurvePart[2]);  
        if (fComputed && dx() > 0 && approximately_zero(dy())) {
            SkDCubic origCurve; 
            int last = fSegment->count() - 1;
            fSegment->subDivide(fStart < fEnd ? 0 : last, fStart < fEnd ? last : 0, &origCurve);
            SkLineParameters origTan;
            origTan.quadEndPoints(*SkTCast<SkDQuad*>(&origCurve));
            if (origTan.dx() <= 0
                    || (dy() != origTan.dy() && dy() * origTan.dy() <= 0)) { 
                fUnorderable = true;
                return;
            }
        }
        } break;
    case SkPath::kCubic_Verb: {
        double startT = fSegment->t(fStart);
        fSide2 = -fTangentHalf.cubicPart(fCurveHalf);
        fTangentPart.cubicEndPoints(fCurvePart);
        double testTs[4];
        
        int testCount = SkDCubic::FindInflections(pts, testTs);
        double endT = fSegment->t(fEnd);
        double limitT = endT;
        int index;
        for (index = 0; index < testCount; ++index) {
            if (!between(startT, testTs[index], limitT)) {
                testTs[index] = -1;
            }
        }
        testTs[testCount++] = startT;
        testTs[testCount++] = endT;
        SkTQSort<double>(testTs, &testTs[testCount - 1]);
        double bestSide = 0;
        int testCases = (testCount << 1) - 1;
        index = 0;
        while (testTs[index] < 0) {
            ++index;
        }
        index <<= 1;
        for (; index < testCases; ++index) {
            int testIndex = index >> 1;
            double testT = testTs[testIndex];
            if (index & 1) {
                testT = (testT + testTs[testIndex + 1]) / 2;
            }
            
            SkDPoint pt = dcubic_xy_at_t(pts, testT);
            double testSide = fTangentPart.pointDistance(pt);
            if (fabs(bestSide) < fabs(testSide)) {
                bestSide = testSide;
            }
        }
        fSide = -bestSide;  
        SkASSERT(fSide == 0 || fSide2 != 0);
        if (fComputed && dx() > 0 && approximately_zero(dy())) {
            SkDCubic origCurve; 
            int last = fSegment->count() - 1;
            fSegment->subDivide(fStart < fEnd ? 0 : last, fStart < fEnd ? last : 0, &origCurve);
            SkDCubicPair split = origCurve.chopAt(startT);
            SkLineParameters splitTan;
            splitTan.cubicEndPoints(fStart < fEnd ? split.second() : split.first());
            if (splitTan.dx() <= 0) {
                fUnorderable = true;
                fUnsortable = fSegment->isTiny(this);
                return;
            }
            
            if (dy() * splitTan.dy() < 0) {
                fUnorderable = true;
                fUnsortable = fSegment->isTiny(this);
                return;
            }
        }
        } break;
    default:
        SkASSERT(0);
    }
    if ((fUnsortable = approximately_zero(dx()) && approximately_zero(dy()))) {
        return;
    }
    if (fSegment->verb() == SkPath::kLine_Verb) {
        return;
    }
    SkASSERT(fStart != fEnd);
    int smaller = SkMin32(fStart, fEnd);
    int larger = SkMax32(fStart, fEnd);
    while (smaller < larger && fSegment->span(smaller).fTiny) {
        ++smaller;
    }
    if (precisely_equal(fSegment->span(smaller).fT, fSegment->span(larger).fT)) {
    #if DEBUG_UNSORTABLE
        SkPoint iPt = fSegment->xyAtT(fStart);
        SkPoint ePt = fSegment->xyAtT(fEnd);
        SkDebugf("%s all tiny unsortable [%d] (%1.9g,%1.9g) [%d] (%1.9g,%1.9g)\n", __FUNCTION__,
            fStart, iPt.fX, iPt.fY, fEnd, ePt.fX, ePt.fY);
    #endif
        fUnsortable = true;
        return;
    }
    fUnsortable = fStart < fEnd ? fSegment->span(smaller).fUnsortableStart
            : fSegment->span(larger).fUnsortableEnd;
#if DEBUG_UNSORTABLE
    if (fUnsortable) {
        SkPoint iPt = fSegment->xyAtT(smaller);
        SkPoint ePt = fSegment->xyAtT(larger);
        SkDebugf("%s unsortable [%d] (%1.9g,%1.9g) [%d] (%1.9g,%1.9g)\n", __FUNCTION__,
                smaller, iPt.fX, iPt.fY, fEnd, ePt.fX, ePt.fY);
    }
#endif
    return;
}

#ifdef SK_DEBUG
void SkOpAngle::dump() const {
    const SkOpSpan& spanStart = fSegment->span(fStart);
    const SkOpSpan& spanEnd = fSegment->span(fEnd);
    const SkOpSpan& spanMin = fStart < fEnd ? spanStart : spanEnd;
    SkDebugf("id=%d (%1.9g,%1.9g) start=%d (%1.9g) end=%d (%1.9g) sumWind=",
            fSegment->debugID(), fSegment->xAtT(fStart), fSegment->yAtT(fStart),
            fStart, spanStart.fT, fEnd, spanEnd.fT);
    SkPathOpsDebug::WindingPrintf(spanMin.fWindSum);
    SkDebugf(" oppWind=");
    SkPathOpsDebug::WindingPrintf(spanMin.fOppSum),
    SkDebugf(" done=%d\n", spanMin.fDone);
}
#endif
