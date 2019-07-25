









#ifndef GrColor_DEFINED
#define GrColor_DEFINED

#include "GrTypes.h"





typedef uint32_t GrColor;



#define GrColor_INDEX_R     0
#define GrColor_INDEX_G     1
#define GrColor_INDEX_B     2
#define GrColor_INDEX_A     3



#define GrColor_SHIFT_R     0
#define GrColor_SHIFT_G     8
#define GrColor_SHIFT_B     16
#define GrColor_SHIFT_A     24




static inline GrColor GrColorPackRGBA(unsigned r, unsigned g,
                                      unsigned b, unsigned a) {
    GrAssert((uint8_t)r == r);
    GrAssert((uint8_t)g == g);
    GrAssert((uint8_t)b == b);
    GrAssert((uint8_t)a == a);
    return  (r << GrColor_SHIFT_R) |
            (g << GrColor_SHIFT_G) |
            (b << GrColor_SHIFT_B) |
            (a << GrColor_SHIFT_A);
}



#define GrColorUnpackR(color)   (((color) >> GrColor_SHIFT_R) & 0xFF)
#define GrColorUnpackG(color)   (((color) >> GrColor_SHIFT_G) & 0xFF)
#define GrColorUnpackB(color)   (((color) >> GrColor_SHIFT_B) & 0xFF)
#define GrColorUnpackA(color)   (((color) >> GrColor_SHIFT_A) & 0xFF)





#define GrColor_ILLEGAL     (~(0xFF << GrColor_SHIFT_A))

#endif

