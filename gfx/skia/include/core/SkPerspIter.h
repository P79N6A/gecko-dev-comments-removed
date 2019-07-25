








#ifndef SkPerspIter_DEFINED
#define SkPerspIter_DEFINED

#include "SkMatrix.h"

class SkPerspIter {
public:
    





    SkPerspIter(const SkMatrix& m, SkScalar x, SkScalar y, int count);
    
    



    const SkFixed* getXY() const { return fStorage; }

    


    int next();
    
private:
    enum {
        kShift  = 4,
        kCount  = (1 << kShift)
    };
    const SkMatrix& fMatrix;
    SkFixed         fStorage[kCount * 2];
    SkFixed         fX, fY;
    SkScalar        fSX, fSY;
    int             fCount;
};

#endif
