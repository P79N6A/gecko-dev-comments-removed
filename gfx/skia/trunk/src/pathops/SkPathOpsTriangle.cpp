






#include "SkPathOpsTriangle.h"



bool SkDTriangle::contains(const SkDPoint& pt) const {

    SkDVector v0 = fPts[2] - fPts[0];
    SkDVector v1 = fPts[1] - fPts[0];
    SkDVector v2 = pt - fPts[0];


    double dot00 = v0.dot(v0);
    double dot01 = v0.dot(v1);
    double dot02 = v0.dot(v2);
    double dot11 = v1.dot(v1);
    double dot12 = v1.dot(v2);



#if 0

    double invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
    double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    double v = (dot00 * dot12 - dot01 * dot02) * invDenom;


    return (u >= 0) && (v >= 0) && (u + v <= 1);
#else
    double w = dot00 * dot11 - dot01 * dot01;
    if (w == 0) {
        return false;
    }
    double wSign = w < 0 ? -1 : 1;
    double u = (dot11 * dot02 - dot01 * dot12) * wSign;
    if (u <= 0) {
        return false;
    }
    double v = (dot00 * dot12 - dot01 * dot02) * wSign;
    if (v <= 0) {
        return false;
    }
    return u + v < w * wSign;
#endif
}
