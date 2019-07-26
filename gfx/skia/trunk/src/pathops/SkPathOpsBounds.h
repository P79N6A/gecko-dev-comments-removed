





#ifndef SkPathOpBounds_DEFINED
#define SkPathOpBounds_DEFINED

#include "SkPathOpsRect.h"
#include "SkRect.h"


struct SkPathOpsBounds : public SkRect {
    static bool Intersects(const SkPathOpsBounds& a, const SkPathOpsBounds& b) {
        return AlmostLessOrEqualUlps(a.fLeft, b.fRight)
                && AlmostLessOrEqualUlps(b.fLeft, a.fRight)
                && AlmostLessOrEqualUlps(a.fTop, b.fBottom)
                && AlmostLessOrEqualUlps(b.fTop, a.fBottom);
    }

   
   
   
    void add(SkScalar left, SkScalar top, SkScalar right, SkScalar bottom) {
        if (left < fLeft) fLeft = left;
        if (top < fTop) fTop = top;
        if (right > fRight) fRight = right;
        if (bottom > fBottom) fBottom = bottom;
    }

    void add(const SkPathOpsBounds& toAdd) {
        add(toAdd.fLeft, toAdd.fTop, toAdd.fRight, toAdd.fBottom);
    }

    void add(const SkPoint& pt) {
        if (pt.fX < fLeft) fLeft = pt.fX;
        if (pt.fY < fTop) fTop = pt.fY;
        if (pt.fX > fRight) fRight = pt.fX;
        if (pt.fY > fBottom) fBottom = pt.fY;
    }

    bool almostContains(const SkPoint& pt) {
        return AlmostLessOrEqualUlps(fLeft, pt.fX)
                && AlmostLessOrEqualUlps(pt.fX, fRight)
                && AlmostLessOrEqualUlps(fTop, pt.fY)
                && AlmostLessOrEqualUlps(pt.fY, fBottom);
    }

    
    
    bool isReallyEmpty() const {
        
        return !(fLeft <= fRight) || !(fTop <= fBottom)
                || (fLeft == fRight && fTop == fBottom);
    }

    void setCubicBounds(const SkPoint a[4]);
    void setLineBounds(const SkPoint a[2]);
    void setQuadBounds(const SkPoint a[3]);

    void setPointBounds(const SkPoint& pt) {
        fLeft = fRight = pt.fX;
        fTop = fBottom = pt.fY;
    }

    typedef SkRect INHERITED;
};

extern void (SkPathOpsBounds::*SetCurveBounds[])(const SkPoint[]);

#endif
