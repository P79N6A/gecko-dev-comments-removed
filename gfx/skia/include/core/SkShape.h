






#ifndef SkShape_DEFINED
#define SkShape_DEFINED

#include "SkFlattenable.h"

class SkCanvas;
class SkMatrix;
class SkWStream;

class SkShape : public SkFlattenable {
public:
    SkShape();
    virtual ~SkShape();

    void draw(SkCanvas*);

    


    void drawXY(SkCanvas*, SkScalar dx, SkScalar dy);

    


    void drawMatrix(SkCanvas*, const SkMatrix&);

    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkShape)

protected:
    virtual void onDraw(SkCanvas*);

    SkShape(SkFlattenableReadBuffer&);

private:

    typedef SkFlattenable INHERITED;
};

#endif
