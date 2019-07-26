






#ifndef SkGradientShader_DEFINED
#define SkGradientShader_DEFINED

#include "SkShader.h"

class SkUnitMapper;






class SK_API SkGradientShader {
public:
    enum Flags {
        




        kInterpolateColorsInPremul_Flag = 1 << 0,
    };

    
















    static SkShader* CreateLinear(const SkPoint pts[2],
                                  const SkColor colors[], const SkScalar pos[], int count,
                                  SkShader::TileMode mode,
                                  SkUnitMapper* mapper = NULL,
                                  uint32_t flags = 0);

    
















    static SkShader* CreateRadial(const SkPoint& center, SkScalar radius,
                                  const SkColor colors[], const SkScalar pos[], int count,
                                  SkShader::TileMode mode,
                                  SkUnitMapper* mapper = NULL,
                                  uint32_t flags = 0);

    



















    static SkShader* CreateTwoPointRadial(const SkPoint& start,
                                          SkScalar startRadius,
                                          const SkPoint& end,
                                          SkScalar endRadius,
                                          const SkColor colors[],
                                          const SkScalar pos[], int count,
                                          SkShader::TileMode mode,
                                          SkUnitMapper* mapper = NULL,
                                          uint32_t flags = 0);

    





    static SkShader* CreateTwoPointConical(const SkPoint& start,
                                           SkScalar startRadius,
                                           const SkPoint& end,
                                           SkScalar endRadius,
                                           const SkColor colors[],
                                           const SkScalar pos[], int count,
                                           SkShader::TileMode mode,
                                           SkUnitMapper* mapper = NULL,
                                           uint32_t flags = 0);

    















    static SkShader* CreateSweep(SkScalar cx, SkScalar cy,
                                 const SkColor colors[], const SkScalar pos[],
                                 int count, SkUnitMapper* mapper = NULL,
                                 uint32_t flags = 0);

    SK_DECLARE_FLATTENABLE_REGISTRAR_GROUP()
};

#endif
