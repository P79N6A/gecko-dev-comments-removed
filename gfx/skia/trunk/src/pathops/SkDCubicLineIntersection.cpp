





#include "SkIntersections.h"
#include "SkPathOpsCubic.h"
#include "SkPathOpsLine.h"



































































class LineCubicIntersections {
public:
    enum PinTPoint {
        kPointUninitialized,
        kPointInitialized
    };

    LineCubicIntersections(const SkDCubic& c, const SkDLine& l, SkIntersections* i)
        : fCubic(c)
        , fLine(l)
        , fIntersections(i)
        , fAllowNear(true) {
        i->setMax(3);
    }

    void allowNear(bool allow) {
        fAllowNear = allow;
    }

    
    int intersectRay(double roots[3]) {
        double adj = fLine[1].fX - fLine[0].fX;
        double opp = fLine[1].fY - fLine[0].fY;
        SkDCubic r;
        for (int n = 0; n < 4; ++n) {
            r[n].fX = (fCubic[n].fY - fLine[0].fY) * adj - (fCubic[n].fX - fLine[0].fX) * opp;
        }
        double A, B, C, D;
        SkDCubic::Coefficients(&r[0].fX, &A, &B, &C, &D);
        return SkDCubic::RootsValidT(A, B, C, D, roots);
    }

    int intersect() {
        addExactEndPoints();
        if (fAllowNear) {
            addNearEndPoints();
        }
        double rootVals[3];
        int roots = intersectRay(rootVals);
        for (int index = 0; index < roots; ++index) {
            double cubicT = rootVals[index];
            double lineT = findLineT(cubicT);
            SkDPoint pt;
            if (pinTs(&cubicT, &lineT, &pt, kPointUninitialized)) {
    #if ONE_OFF_DEBUG
                SkDPoint cPt = fCubic.ptAtT(cubicT);
                SkDebugf("%s pt=(%1.9g,%1.9g) cPt=(%1.9g,%1.9g)\n", __FUNCTION__, pt.fX, pt.fY,
                        cPt.fX, cPt.fY);
    #endif
                for (int inner = 0; inner < fIntersections->used(); ++inner) {
                    if (fIntersections->pt(inner) != pt) {
                        continue;
                    }
                    double existingCubicT = (*fIntersections)[0][inner];
                    if (cubicT == existingCubicT) {
                        goto skipInsert;
                    }
                    
                    double cubicMidT = (existingCubicT + cubicT) / 2;
                    SkDPoint cubicMidPt = fCubic.ptAtT(cubicMidT);
                    if (cubicMidPt.approximatelyEqual(pt)) {
                        goto skipInsert;
                    }
                }
                fIntersections->insert(cubicT, lineT, pt);
        skipInsert:
                ;
            }
        }
        return fIntersections->used();
    }

    int horizontalIntersect(double axisIntercept, double roots[3]) {
        double A, B, C, D;
        SkDCubic::Coefficients(&fCubic[0].fY, &A, &B, &C, &D);
        D -= axisIntercept;
        return SkDCubic::RootsValidT(A, B, C, D, roots);
    }

    int horizontalIntersect(double axisIntercept, double left, double right, bool flipped) {
        addExactHorizontalEndPoints(left, right, axisIntercept);
        if (fAllowNear) {
            addNearHorizontalEndPoints(left, right, axisIntercept);
        }
        double rootVals[3];
        int roots = horizontalIntersect(axisIntercept, rootVals);
        for (int index = 0; index < roots; ++index) {
            double cubicT = rootVals[index];
            SkDPoint pt = fCubic.ptAtT(cubicT);
            double lineT = (pt.fX - left) / (right - left);
            if (pinTs(&cubicT, &lineT, &pt, kPointInitialized)) {
                fIntersections->insert(cubicT, lineT, pt);
            }
        }
        if (flipped) {
            fIntersections->flip();
        }
        return fIntersections->used();
    }

    int verticalIntersect(double axisIntercept, double roots[3]) {
        double A, B, C, D;
        SkDCubic::Coefficients(&fCubic[0].fX, &A, &B, &C, &D);
        D -= axisIntercept;
        return SkDCubic::RootsValidT(A, B, C, D, roots);
    }

    int verticalIntersect(double axisIntercept, double top, double bottom, bool flipped) {
        addExactVerticalEndPoints(top, bottom, axisIntercept);
        if (fAllowNear) {
            addNearVerticalEndPoints(top, bottom, axisIntercept);
        }
        double rootVals[3];
        int roots = verticalIntersect(axisIntercept, rootVals);
        for (int index = 0; index < roots; ++index) {
            double cubicT = rootVals[index];
            SkDPoint pt = fCubic.ptAtT(cubicT);
            double lineT = (pt.fY - top) / (bottom - top);
            if (pinTs(&cubicT, &lineT, &pt, kPointInitialized)) {
                fIntersections->insert(cubicT, lineT, pt);
            }
        }
        if (flipped) {
            fIntersections->flip();
        }
        return fIntersections->used();
    }

    protected:

    void addExactEndPoints() {
        for (int cIndex = 0; cIndex < 4; cIndex += 3) {
            double lineT = fLine.exactPoint(fCubic[cIndex]);
            if (lineT < 0) {
                continue;
            }
            double cubicT = (double) (cIndex >> 1);
            fIntersections->insert(cubicT, lineT, fCubic[cIndex]);
        }
    }

    

    void addNearEndPoints() {
        for (int cIndex = 0; cIndex < 4; cIndex += 3) {
            double cubicT = (double) (cIndex >> 1);
            if (fIntersections->hasT(cubicT)) {
                continue;
            }
            double lineT = fLine.nearPoint(fCubic[cIndex]);
            if (lineT < 0) {
                continue;
            }
            fIntersections->insert(cubicT, lineT, fCubic[cIndex]);
        }
    }

    void addExactHorizontalEndPoints(double left, double right, double y) {
        for (int cIndex = 0; cIndex < 4; cIndex += 3) {
            double lineT = SkDLine::ExactPointH(fCubic[cIndex], left, right, y);
            if (lineT < 0) {
                continue;
            }
            double cubicT = (double) (cIndex >> 1);
            fIntersections->insert(cubicT, lineT, fCubic[cIndex]);
        }
    }

    void addNearHorizontalEndPoints(double left, double right, double y) {
        for (int cIndex = 0; cIndex < 4; cIndex += 3) {
            double cubicT = (double) (cIndex >> 1);
            if (fIntersections->hasT(cubicT)) {
                continue;
            }
            double lineT = SkDLine::NearPointH(fCubic[cIndex], left, right, y);
            if (lineT < 0) {
                continue;
            }
            fIntersections->insert(cubicT, lineT, fCubic[cIndex]);
        }
        
    }

    void addExactVerticalEndPoints(double top, double bottom, double x) {
        for (int cIndex = 0; cIndex < 4; cIndex += 3) {
            double lineT = SkDLine::ExactPointV(fCubic[cIndex], top, bottom, x);
            if (lineT < 0) {
                continue;
            }
            double cubicT = (double) (cIndex >> 1);
            fIntersections->insert(cubicT, lineT, fCubic[cIndex]);
        }
    }

    void addNearVerticalEndPoints(double top, double bottom, double x) {
        for (int cIndex = 0; cIndex < 4; cIndex += 3) {
            double cubicT = (double) (cIndex >> 1);
            if (fIntersections->hasT(cubicT)) {
                continue;
            }
            double lineT = SkDLine::NearPointV(fCubic[cIndex], top, bottom, x);
            if (lineT < 0) {
                continue;
            }
            fIntersections->insert(cubicT, lineT, fCubic[cIndex]);
        }
        
    }

    double findLineT(double t) {
        SkDPoint xy = fCubic.ptAtT(t);
        double dx = fLine[1].fX - fLine[0].fX;
        double dy = fLine[1].fY - fLine[0].fY;
        if (fabs(dx) > fabs(dy)) {
            return (xy.fX - fLine[0].fX) / dx;
        }
        return (xy.fY - fLine[0].fY) / dy;
    }

    bool pinTs(double* cubicT, double* lineT, SkDPoint* pt, PinTPoint ptSet) {
        if (!approximately_one_or_less(*lineT)) {
            return false;
        }
        if (!approximately_zero_or_more(*lineT)) {
            return false;
        }
        double cT = *cubicT = SkPinT(*cubicT);
        double lT = *lineT = SkPinT(*lineT);
        if (lT == 0 || lT == 1 || (ptSet == kPointUninitialized && cT != 0 && cT != 1)) {
            *pt = fLine.ptAtT(lT);
        } else if (ptSet == kPointUninitialized) {
            *pt = fCubic.ptAtT(cT);
        }
        SkPoint gridPt = pt->asSkPoint();
        if (gridPt == fLine[0].asSkPoint()) {
            *lineT = 0;
        } else if (gridPt == fLine[1].asSkPoint()) {
            *lineT = 1;
        }
        if (gridPt == fCubic[0].asSkPoint() && approximately_equal(*cubicT, 0)) {
            *cubicT = 0;
        } else if (gridPt == fCubic[3].asSkPoint() && approximately_equal(*cubicT, 1)) {
            *cubicT = 1;
        }
        return true;
    }

private:
    const SkDCubic& fCubic;
    const SkDLine& fLine;
    SkIntersections* fIntersections;
    bool fAllowNear;
};

int SkIntersections::horizontal(const SkDCubic& cubic, double left, double right, double y,
        bool flipped) {
    SkDLine line = {{{ left, y }, { right, y }}};
    LineCubicIntersections c(cubic, line, this);
    return c.horizontalIntersect(y, left, right, flipped);
}

int SkIntersections::vertical(const SkDCubic& cubic, double top, double bottom, double x,
        bool flipped) {
    SkDLine line = {{{ x, top }, { x, bottom }}};
    LineCubicIntersections c(cubic, line, this);
    return c.verticalIntersect(x, top, bottom, flipped);
}

int SkIntersections::intersect(const SkDCubic& cubic, const SkDLine& line) {
    LineCubicIntersections c(cubic, line, this);
    c.allowNear(fAllowNear);
    return c.intersect();
}

int SkIntersections::intersectRay(const SkDCubic& cubic, const SkDLine& line) {
    LineCubicIntersections c(cubic, line, this);
    fUsed = c.intersectRay(fT[0]);
    for (int index = 0; index < fUsed; ++index) {
        fPt[index] = cubic.ptAtT(fT[0][index]);
    }
    return fUsed;
}
