








#ifndef SkCubicClipper_DEFINED
#define SkCubicClipper_DEFINED

#include "SkPoint.h"
#include "SkRect.h"







class SkCubicClipper {
public:
    SkCubicClipper();

    void setClip(const SkIRect& clip);

    bool clipCubic(const SkPoint src[4], SkPoint dst[4]);

private:
    SkRect      fClip;
};

#endif  
