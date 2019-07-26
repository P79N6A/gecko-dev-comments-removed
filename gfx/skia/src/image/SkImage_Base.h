






#ifndef SkImage_Base_DEFINED
#define SkImage_Base_DEFINED

#include "SkImage.h"

class SkImage_Base : public SkImage {
public:
    SkImage_Base(int width, int height) : INHERITED(width, height) {}

    virtual void onDraw(SkCanvas*, SkScalar, SkScalar, const SkPaint*) = 0;

private:
    typedef SkImage INHERITED;
};

#endif

