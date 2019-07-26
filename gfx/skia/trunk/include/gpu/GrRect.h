






#ifndef GrRect_DEFINED
#define GrRect_DEFINED

#include "SkTypes.h"
#include "SkRect.h"

struct GrIRect16 {
    int16_t fLeft, fTop, fRight, fBottom;

    int width() const { return fRight - fLeft; }
    int height() const { return fBottom - fTop; }
    int area() const { return this->width() * this->height(); }
    bool isEmpty() const { return fLeft >= fRight || fTop >= fBottom; }

    void set(const SkIRect& r) {
        fLeft   = SkToS16(r.fLeft);
        fTop    = SkToS16(r.fTop);
        fRight  = SkToS16(r.fRight);
        fBottom = SkToS16(r.fBottom);
    }
};

#endif
