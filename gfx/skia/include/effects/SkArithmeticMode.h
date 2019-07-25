






#ifndef SkArithmeticMode_DEFINED
#define SkArithmeticMode_DEFINED

#include "SkXfermode.h"

class SkArithmeticMode : public SkXfermode {
public:
    









    static SkXfermode* Create(SkScalar k1, SkScalar k2,
                              SkScalar k3, SkScalar k4);
};

#endif

