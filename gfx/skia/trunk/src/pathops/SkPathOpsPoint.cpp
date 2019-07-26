





#include "SkPathOpsPoint.h"

SkDVector operator-(const SkDPoint& a, const SkDPoint& b) {
    SkDVector v = {a.fX - b.fX, a.fY - b.fY};
    return v;
}

SkDPoint operator+(const SkDPoint& a, const SkDVector& b) {
    SkDPoint v = {a.fX + b.fX, a.fY + b.fY};
    return v;
}
