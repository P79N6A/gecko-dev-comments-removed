





#include "SkPathOpsLine.h"

SkDLine SkDLine::subDivide(double t1, double t2) const {
    SkDVector delta = tangent();
    SkDLine dst = {{{
            fPts[0].fX - t1 * delta.fX, fPts[0].fY - t1 * delta.fY}, {
            fPts[0].fX - t2 * delta.fX, fPts[0].fY - t2 * delta.fY}}};
    return dst;
}






















double SkDLine::isLeft(const SkDPoint& pt) const {
    SkDVector p0 = fPts[1] - fPts[0];
    SkDVector p2 = pt - fPts[0];
    return p0.cross(p2);
}

SkDPoint SkDLine::ptAtT(double t) const {
    if (0 == t) {
        return fPts[0];
    }
    if (1 == t) {
        return fPts[1];
    }
    double one_t = 1 - t;
    SkDPoint result = { one_t * fPts[0].fX + t * fPts[1].fX, one_t * fPts[0].fY + t * fPts[1].fY };
    return result;
}

double SkDLine::exactPoint(const SkDPoint& xy) const {
    if (xy == fPts[0]) {  
        return 0;
    }
    if (xy == fPts[1]) {
        return 1;
    }
    return -1;
}

double SkDLine::nearPoint(const SkDPoint& xy, bool* unequal) const {
    if (!AlmostBetweenUlps(fPts[0].fX, xy.fX, fPts[1].fX)
            || !AlmostBetweenUlps(fPts[0].fY, xy.fY, fPts[1].fY)) {
        return -1;
    }
    
    SkDVector len = fPts[1] - fPts[0]; 
    double denom = len.fX * len.fX + len.fY * len.fY;  
    SkDVector ab0 = xy - fPts[0];
    double numer = len.fX * ab0.fX + ab0.fY * len.fY;
    if (!between(0, numer, denom)) {
        return -1;
    }
    double t = numer / denom;
    SkDPoint realPt = ptAtT(t);
    double dist = realPt.distance(xy);   
    
    double tiniest = SkTMin(SkTMin(SkTMin(fPts[0].fX, fPts[0].fY), fPts[1].fX), fPts[1].fY);
    double largest = SkTMax(SkTMax(SkTMax(fPts[0].fX, fPts[0].fY), fPts[1].fX), fPts[1].fY);
    largest = SkTMax(largest, -tiniest);
    if (!AlmostEqualUlps(largest, largest + dist)) { 
        return -1;
    }
    if (unequal) {
        *unequal = (float) largest != (float) (largest + dist);
    }
    t = SkPinT(t);
    SkASSERT(between(0, t, 1));
    return t;
}

bool SkDLine::nearRay(const SkDPoint& xy) const {
    
    SkDVector len = fPts[1] - fPts[0]; 
    double denom = len.fX * len.fX + len.fY * len.fY;  
    SkDVector ab0 = xy - fPts[0];
    double numer = len.fX * ab0.fX + ab0.fY * len.fY;
    double t = numer / denom;
    SkDPoint realPt = ptAtT(t);
    double dist = realPt.distance(xy);   
    
    double tiniest = SkTMin(SkTMin(SkTMin(fPts[0].fX, fPts[0].fY), fPts[1].fX), fPts[1].fY);
    double largest = SkTMax(SkTMax(SkTMax(fPts[0].fX, fPts[0].fY), fPts[1].fX), fPts[1].fY);
    largest = SkTMax(largest, -tiniest);
    return RoughlyEqualUlps(largest, largest + dist); 
}



bool SkDLine::NearRay(double x1, double y1, double x2, double y2) {
    double denom1 = x1 * x1 + y1 * y1;
    double denom2 = x2 * x2 + y2 * y2;
    SkDLine line = {{{0, 0}, {x1, y1}}};
    SkDPoint pt = {x2, y2};
    if (denom2 > denom1) {
        SkTSwap(line[1], pt);
    }
    return line.nearRay(pt);
}

double SkDLine::ExactPointH(const SkDPoint& xy, double left, double right, double y) {
    if (xy.fY == y) {
        if (xy.fX == left) {
            return 0;
        }
        if (xy.fX == right) {
            return 1;
        }
    }
    return -1;
}

double SkDLine::NearPointH(const SkDPoint& xy, double left, double right, double y) {
    if (!AlmostBequalUlps(xy.fY, y)) {
        return -1;
    }
    if (!AlmostBetweenUlps(left, xy.fX, right)) {
        return -1;
    }
    double t = (xy.fX - left) / (right - left);
    t = SkPinT(t);
    SkASSERT(between(0, t, 1));
    double realPtX = (1 - t) * left + t * right;
    SkDVector distU = {xy.fY - y, xy.fX - realPtX};
    double distSq = distU.fX * distU.fX + distU.fY * distU.fY;
    double dist = sqrt(distSq); 
    double tiniest = SkTMin(SkTMin(y, left), right);
    double largest = SkTMax(SkTMax(y, left), right);
    largest = SkTMax(largest, -tiniest);
    if (!AlmostEqualUlps(largest, largest + dist)) { 
        return -1;
    }
    return t;
}

double SkDLine::ExactPointV(const SkDPoint& xy, double top, double bottom, double x) {
    if (xy.fX == x) {
        if (xy.fY == top) {
            return 0;
        }
        if (xy.fY == bottom) {
            return 1;
        }
    }
    return -1;
}

double SkDLine::NearPointV(const SkDPoint& xy, double top, double bottom, double x) {
    if (!AlmostBequalUlps(xy.fX, x)) {
        return -1;
    }
    if (!AlmostBetweenUlps(top, xy.fY, bottom)) {
        return -1;
    }
    double t = (xy.fY - top) / (bottom - top);
    t = SkPinT(t);
    SkASSERT(between(0, t, 1));
    double realPtY = (1 - t) * top + t * bottom;
    SkDVector distU = {xy.fX - x, xy.fY - realPtY};
    double distSq = distU.fX * distU.fX + distU.fY * distU.fY;
    double dist = sqrt(distSq); 
    double tiniest = SkTMin(SkTMin(x, top), bottom);
    double largest = SkTMax(SkTMax(x, top), bottom);
    largest = SkTMax(largest, -tiniest);
    if (!AlmostEqualUlps(largest, largest + dist)) { 
        return -1;
    }
    return t;
}
