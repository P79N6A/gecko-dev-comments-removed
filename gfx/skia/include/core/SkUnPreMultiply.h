











#ifndef SkUnPreMultiply_DEFINED
#define SkUnPreMultiply_DEFINED

#include "SkColor.h"

class SK_API SkUnPreMultiply {
public:
    typedef uint32_t Scale;

    
    static const Scale* GetScaleTable() {
        return gTable;
    }

    static Scale GetScale(U8CPU alpha) {
        SkASSERT(alpha <= 255);
        return gTable[alpha];
    }

    












    static U8CPU ApplyScale(Scale scale, U8CPU component) {
        SkASSERT(component <= 255);
        return (scale * component + (1 << 23)) >> 24;
    }

    static SkColor PMColorToColor(SkPMColor c);

private:
    static const uint32_t gTable[256];
};

#endif
