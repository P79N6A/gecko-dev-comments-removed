






#ifndef SkArithmeticMode_DEFINED
#define SkArithmeticMode_DEFINED

#include "SkFlattenable.h"
#include "SkScalar.h"

class SkXfermode;

class SK_API SkArithmeticMode {
public:
    









    static SkXfermode* Create(SkScalar k1, SkScalar k2,
                              SkScalar k3, SkScalar k4,
                              bool enforcePMColor = true);

    SK_DECLARE_FLATTENABLE_REGISTRAR_GROUP();

private:
    SkArithmeticMode(); 
};

#endif
