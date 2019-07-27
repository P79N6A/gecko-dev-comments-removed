





#include "SkIntersections.h"
#include "SkPathOpsLine.h"
#include "SkPathOpsQuad.h"















































































class LineQuadraticIntersections {
public:
    enum PinTPoint {
        kPointUninitialized,
        kPointInitialized
    };

    LineQuadraticIntersections(const SkDQuad& q, const SkDLine& l, SkIntersections* i)
        : fQuad(q)
        , fLine(l)
        , fIntersections(i)
        , fAllowNear(true) {
        i->setMax(3);  
    }

    void allowNear(bool allow) {
        fAllowNear = allow;
    }

    int intersectRay(double roots[2]) {
    














        double adj = fLine[1].fX - fLine[0].fX;
        double opp = fLine[1].fY - fLine[0].fY;
        double r[3];
        for (int n = 0; n < 3; ++n) {
            r[n] = (fQuad[n].fY - fLine[0].fY) * adj - (fQuad[n].fX - fLine[0].fX) * opp;
        }
        double A = r[2];
        double B = r[1];
        double C = r[0];
        A += C - 2 * B;  
        B -= C;  
        return SkDQuad::RootsValidT(A, 2 * B, C, roots);
    }

    int intersect() {
        addExactEndPoints();
        if (fAllowNear) {
            addNearEndPoints();
        }
        if (fIntersections->used() == 2) {
            
        } else {
            double rootVals[2];
            int roots = intersectRay(rootVals);
            for (int index = 0; index < roots; ++index) {
                double quadT = rootVals[index];
                double lineT = findLineT(quadT);
                SkDPoint pt;
                if (pinTs(&quadT, &lineT, &pt, kPointUninitialized)) {
                    fIntersections->insert(quadT, lineT, pt);
                }
            }
        }
        return fIntersections->used();
    }

    int horizontalIntersect(double axisIntercept, double roots[2]) {
        double D = fQuad[2].fY;  
        double E = fQuad[1].fY;  
        double F = fQuad[0].fY;  
        D += F - 2 * E;         
        E -= F;                 
        F -= axisIntercept;
        return SkDQuad::RootsValidT(D, 2 * E, F, roots);
    }

    int horizontalIntersect(double axisIntercept, double left, double right, bool flipped) {
        addExactHorizontalEndPoints(left, right, axisIntercept);
        if (fAllowNear) {
            addNearHorizontalEndPoints(left, right, axisIntercept);
        }
        double rootVals[2];
        int roots = horizontalIntersect(axisIntercept, rootVals);
        for (int index = 0; index < roots; ++index) {
            double quadT = rootVals[index];
            SkDPoint pt = fQuad.ptAtT(quadT);
            double lineT = (pt.fX - left) / (right - left);
            if (pinTs(&quadT, &lineT, &pt, kPointInitialized)) {
                fIntersections->insert(quadT, lineT, pt);
            }
        }
        if (flipped) {
            fIntersections->flip();
        }
        return fIntersections->used();
    }

    int verticalIntersect(double axisIntercept, double roots[2]) {
        double D = fQuad[2].fX;  
        double E = fQuad[1].fX;  
        double F = fQuad[0].fX;  
        D += F - 2 * E;         
        E -= F;                 
        F -= axisIntercept;
        return SkDQuad::RootsValidT(D, 2 * E, F, roots);
    }

    int verticalIntersect(double axisIntercept, double top, double bottom, bool flipped) {
        addExactVerticalEndPoints(top, bottom, axisIntercept);
        if (fAllowNear) {
            addNearVerticalEndPoints(top, bottom, axisIntercept);
        }
        double rootVals[2];
        int roots = verticalIntersect(axisIntercept, rootVals);
        for (int index = 0; index < roots; ++index) {
            double quadT = rootVals[index];
            SkDPoint pt = fQuad.ptAtT(quadT);
            double lineT = (pt.fY - top) / (bottom - top);
            if (pinTs(&quadT, &lineT, &pt, kPointInitialized)) {
                fIntersections->insert(quadT, lineT, pt);
            }
        }
        if (flipped) {
            fIntersections->flip();
        }
        return fIntersections->used();
    }

protected:
    
    void addExactEndPoints() {
        for (int qIndex = 0; qIndex < 3; qIndex += 2) {
            double lineT = fLine.exactPoint(fQuad[qIndex]);
            if (lineT < 0) {
                continue;
            }
            double quadT = (double) (qIndex >> 1);
            fIntersections->insert(quadT, lineT, fQuad[qIndex]);
        }
    }

    void addNearEndPoints() {
        for (int qIndex = 0; qIndex < 3; qIndex += 2) {
            double quadT = (double) (qIndex >> 1);
            if (fIntersections->hasT(quadT)) {
                continue;
            }
            double lineT = fLine.nearPoint(fQuad[qIndex], NULL);
            if (lineT < 0) {
                continue;
            }
            fIntersections->insert(quadT, lineT, fQuad[qIndex]);
        }
        
    }

    void addExactHorizontalEndPoints(double left, double right, double y) {
        for (int qIndex = 0; qIndex < 3; qIndex += 2) {
            double lineT = SkDLine::ExactPointH(fQuad[qIndex], left, right, y);
            if (lineT < 0) {
                continue;
            }
            double quadT = (double) (qIndex >> 1);
            fIntersections->insert(quadT, lineT, fQuad[qIndex]);
        }
    }

    void addNearHorizontalEndPoints(double left, double right, double y) {
        for (int qIndex = 0; qIndex < 3; qIndex += 2) {
            double quadT = (double) (qIndex >> 1);
            if (fIntersections->hasT(quadT)) {
                continue;
            }
            double lineT = SkDLine::NearPointH(fQuad[qIndex], left, right, y);
            if (lineT < 0) {
                continue;
            }
            fIntersections->insert(quadT, lineT, fQuad[qIndex]);
        }
        
    }

    void addExactVerticalEndPoints(double top, double bottom, double x) {
        for (int qIndex = 0; qIndex < 3; qIndex += 2) {
            double lineT = SkDLine::ExactPointV(fQuad[qIndex], top, bottom, x);
            if (lineT < 0) {
                continue;
            }
            double quadT = (double) (qIndex >> 1);
            fIntersections->insert(quadT, lineT, fQuad[qIndex]);
        }
    }

    void addNearVerticalEndPoints(double top, double bottom, double x) {
        for (int qIndex = 0; qIndex < 3; qIndex += 2) {
            double quadT = (double) (qIndex >> 1);
            if (fIntersections->hasT(quadT)) {
                continue;
            }
            double lineT = SkDLine::NearPointV(fQuad[qIndex], top, bottom, x);
            if (lineT < 0) {
                continue;
            }
            fIntersections->insert(quadT, lineT, fQuad[qIndex]);
        }
        
    }

    double findLineT(double t) {
        SkDPoint xy = fQuad.ptAtT(t);
        double dx = fLine[1].fX - fLine[0].fX;
        double dy = fLine[1].fY - fLine[0].fY;
        if (fabs(dx) > fabs(dy)) {
            return (xy.fX - fLine[0].fX) / dx;
        }
        return (xy.fY - fLine[0].fY) / dy;
    }

    bool pinTs(double* quadT, double* lineT, SkDPoint* pt, PinTPoint ptSet) {
        if (!approximately_one_or_less_double(*lineT)) {
            return false;
        }
        if (!approximately_zero_or_more_double(*lineT)) {
            return false;
        }
        double qT = *quadT = SkPinT(*quadT);
        double lT = *lineT = SkPinT(*lineT);
        if (lT == 0 || lT == 1 || (ptSet == kPointUninitialized && qT != 0 && qT != 1)) {
            *pt = fLine.ptAtT(lT);
        } else if (ptSet == kPointUninitialized) {
            *pt = fQuad.ptAtT(qT);
        }
        SkPoint gridPt = pt->asSkPoint();
        if (SkDPoint::ApproximatelyEqual(gridPt, fLine[0].asSkPoint())) {
            *pt = fLine[0];
            *lineT = 0;
        } else if (SkDPoint::ApproximatelyEqual(gridPt, fLine[1].asSkPoint())) {
            *pt = fLine[1];
            *lineT = 1;
        }
        if (fIntersections->used() > 0 && approximately_equal((*fIntersections)[1][0], *lineT)) {
            return false;
        }
        if (gridPt == fQuad[0].asSkPoint()) {
            *pt = fQuad[0];
            *quadT = 0;
        } else if (gridPt == fQuad[2].asSkPoint()) {
            *pt = fQuad[2];
            *quadT = 1;
        }
        return true;
    }

private:
    const SkDQuad& fQuad;
    const SkDLine& fLine;
    SkIntersections* fIntersections;
    bool fAllowNear;
};

int SkIntersections::horizontal(const SkDQuad& quad, double left, double right, double y,
                                bool flipped) {
    SkDLine line = {{{ left, y }, { right, y }}};
    LineQuadraticIntersections q(quad, line, this);
    return q.horizontalIntersect(y, left, right, flipped);
}

int SkIntersections::vertical(const SkDQuad& quad, double top, double bottom, double x,
                              bool flipped) {
    SkDLine line = {{{ x, top }, { x, bottom }}};
    LineQuadraticIntersections q(quad, line, this);
    return q.verticalIntersect(x, top, bottom, flipped);
}

int SkIntersections::intersect(const SkDQuad& quad, const SkDLine& line) {
    LineQuadraticIntersections q(quad, line, this);
    q.allowNear(fAllowNear);
    return q.intersect();
}

int SkIntersections::intersectRay(const SkDQuad& quad, const SkDLine& line) {
    LineQuadraticIntersections q(quad, line, this);
    fUsed = q.intersectRay(fT[0]);
    for (int index = 0; index < fUsed; ++index) {
        fPt[index] = quad.ptAtT(fT[0][index]);
    }
    return fUsed;
}
