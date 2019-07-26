






#ifndef SkMatrixUtils_DEFINED
#define SkMatrixUtils_DEFINED

#include "SkMatrix.h"





#define kSkSubPixelBitsForBilerp   4













bool SkTreatAsSprite(const SkMatrix&, int width, int height,
                     unsigned subpixelBits);





static inline bool SkTreatAsSpriteFilter(const SkMatrix& matrix,
                                         int width, int height) {
    return SkTreatAsSprite(matrix, width, height, kSkSubPixelBitsForBilerp);
}







bool SkDecomposeUpper2x2(const SkMatrix& matrix,
                         SkPoint* rotation1,
                         SkPoint* scale,
                         SkPoint* rotation2);

#endif
