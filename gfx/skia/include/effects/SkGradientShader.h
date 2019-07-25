








#ifndef SkGradientShader_DEFINED
#define SkGradientShader_DEFINED

#include "SkShader.h"

class SkUnitMapper;






class SK_API SkGradientShader {
public:
    
















    static SkShader* CreateLinear(  const SkPoint pts[2],
                                    const SkColor colors[], const SkScalar pos[], int count,
                                    SkShader::TileMode mode,
                                    SkUnitMapper* mapper = NULL);

    
















    static SkShader* CreateRadial(  const SkPoint& center, SkScalar radius,
                                    const SkColor colors[], const SkScalar pos[], int count,
                                    SkShader::TileMode mode,
                                    SkUnitMapper* mapper = NULL);

    



















    static SkShader* CreateTwoPointRadial(const SkPoint& start,
                                          SkScalar startRadius,
                                          const SkPoint& end,
                                          SkScalar endRadius,
                                          const SkColor colors[],
                                          const SkScalar pos[], int count,
                                          SkShader::TileMode mode,
                                          SkUnitMapper* mapper = NULL);
    















    static SkShader* CreateSweep(SkScalar cx, SkScalar cy,
                                 const SkColor colors[], const SkScalar pos[],
                                 int count, SkUnitMapper* mapper = NULL);
};

#endif

