






#include "SkGradientShaderPriv.h"



SkFixed clamp_tileproc(SkFixed x) {
    return SkClampMax(x, 0xFFFF);
}



SkFixed repeat_tileproc(SkFixed x) {
    return x & 0xFFFF;
}





#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#pragma optimize("", off)
#endif

SkFixed mirror_tileproc(SkFixed x) {
    int s = x << 15 >> 31;
    return (x ^ s) & 0xFFFF;
}

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#pragma optimize("", on)
#endif

