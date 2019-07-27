





#include "SkOpEdgeBuilder.h"
#include "SkPathOpsCommon.h"

bool TightBounds(const SkPath& path, SkRect* result) {
    
    SkTArray<SkOpContour> contours;
    SkOpEdgeBuilder builder(path, contours);
    if (!builder.finish()) {
        return false;
    }
    SkTArray<SkOpContour*, true> contourList;
    MakeContourList(contours, contourList, false, false);
    SkOpContour** currentPtr = contourList.begin();
    result->setEmpty();
    if (!currentPtr) {
        return true;
    }
    SkOpContour** listEnd = contourList.end();
    SkOpContour* current = *currentPtr++;
    SkPathOpsBounds bounds = current->bounds();
    while (currentPtr != listEnd) {
        current = *currentPtr++;
        bounds.add(current->bounds());
    }
    *result = bounds;
    return true;
}
