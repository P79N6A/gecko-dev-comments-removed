






#ifndef SkPathOpsTriangle_DEFINED
#define SkPathOpsTriangle_DEFINED

#include "SkPathOpsPoint.h"

struct SkDTriangle {
    SkDPoint fPts[3];

    bool contains(const SkDPoint& pt) const;

};

#endif
