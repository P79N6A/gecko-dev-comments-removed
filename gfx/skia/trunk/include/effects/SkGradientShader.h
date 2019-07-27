






#ifndef SkGradientShader_DEFINED
#define SkGradientShader_DEFINED

#include "SkShader.h"

#define SK_SUPPORT_LEGACY_GRADIENT_FACTORIES






class SK_API SkGradientShader {
public:
    enum Flags {
        




        kInterpolateColorsInPremul_Flag = 1 << 0,
    };

    















    static SkShader* CreateLinear(const SkPoint pts[2],
                                  const SkColor colors[], const SkScalar pos[], int count,
                                  SkShader::TileMode mode,
                                  uint32_t flags, const SkMatrix* localMatrix);

    static SkShader* CreateLinear(const SkPoint pts[2],
                                  const SkColor colors[], const SkScalar pos[], int count,
                                  SkShader::TileMode mode) {
        return CreateLinear(pts, colors, pos, count, mode, 0, NULL);
    }

#ifdef SK_SUPPORT_LEGACY_GRADIENT_FACTORIES
    static SkShader* CreateLinear(const SkPoint pts[2],
                                  const SkColor colors[], const SkScalar pos[], int count,
                                  SkShader::TileMode mode, void* ignored,
                                  uint32_t flags, const SkMatrix* localMatrix) {
        return CreateLinear(pts, colors, pos, count, mode, flags, localMatrix);
    }
#endif

    















    static SkShader* CreateRadial(const SkPoint& center, SkScalar radius,
                                  const SkColor colors[], const SkScalar pos[], int count,
                                  SkShader::TileMode mode,
                                  uint32_t flags, const SkMatrix* localMatrix);

    static SkShader* CreateRadial(const SkPoint& center, SkScalar radius,
                                  const SkColor colors[], const SkScalar pos[], int count,
                                  SkShader::TileMode mode) {
        return CreateRadial(center, radius, colors, pos, count, mode, 0, NULL);
    }

#ifdef SK_SUPPORT_LEGACY_GRADIENT_FACTORIES
    static SkShader* CreateRadial(const SkPoint& center, SkScalar radius,
                                  const SkColor colors[], const SkScalar pos[], int count,
                                  SkShader::TileMode mode, void* ignored,
                                  uint32_t flags, const SkMatrix* localMatrix) {
        return CreateRadial(center, radius, colors, pos, count, mode, flags, localMatrix);
    }
#endif

    


















    static SkShader* CreateTwoPointRadial(const SkPoint& start, SkScalar startRadius,
                                          const SkPoint& end, SkScalar endRadius,
                                          const SkColor colors[], const SkScalar pos[], int count,
                                          SkShader::TileMode mode,
                                          uint32_t flags, const SkMatrix* localMatrix);

    static SkShader* CreateTwoPointRadial(const SkPoint& start, SkScalar startRadius,
                                          const SkPoint& end, SkScalar endRadius,
                                          const SkColor colors[], const SkScalar pos[], int count,
                                          SkShader::TileMode mode) {
        return CreateTwoPointRadial(start, startRadius, end, endRadius, colors, pos, count, mode,
                                    0, NULL);
    }

#ifdef SK_SUPPORT_LEGACY_GRADIENT_FACTORIES
    static SkShader* CreateTwoPointRadial(const SkPoint& start, SkScalar startRadius,
                                          const SkPoint& end, SkScalar endRadius,
                                          const SkColor colors[], const SkScalar pos[], int count,
                                          SkShader::TileMode mode, void* ignored,
                                          uint32_t flags, const SkMatrix* localMatrix) {
        return CreateTwoPointRadial(start, startRadius, end, endRadius, colors, pos, count, mode,
                                    flags, localMatrix);
    }
#endif

    





    static SkShader* CreateTwoPointConical(const SkPoint& start, SkScalar startRadius,
                                           const SkPoint& end, SkScalar endRadius,
                                           const SkColor colors[], const SkScalar pos[], int count,
                                           SkShader::TileMode mode,
                                           uint32_t flags, const SkMatrix* localMatrix);

    static SkShader* CreateTwoPointConical(const SkPoint& start, SkScalar startRadius,
                                           const SkPoint& end, SkScalar endRadius,
                                           const SkColor colors[], const SkScalar pos[], int count,
                                           SkShader::TileMode mode) {
        return CreateTwoPointConical(start, startRadius, end, endRadius, colors, pos, count, mode,
                                     0, NULL);
    }

#ifdef SK_SUPPORT_LEGACY_GRADIENT_FACTORIES
    static SkShader* CreateTwoPointConical(const SkPoint& start, SkScalar startRadius,
                                           const SkPoint& end, SkScalar endRadius,
                                           const SkColor colors[], const SkScalar pos[], int count,
                                           SkShader::TileMode mode, void* ignored,
                                           uint32_t flags, const SkMatrix* localMatrix) {
        return CreateTwoPointConical(start, startRadius, end, endRadius, colors, pos, count, mode,
                                    flags, localMatrix);
    }
#endif

    














    static SkShader* CreateSweep(SkScalar cx, SkScalar cy,
                                 const SkColor colors[], const SkScalar pos[], int count,
                                 uint32_t flags, const SkMatrix* localMatrix);

    static SkShader* CreateSweep(SkScalar cx, SkScalar cy,
                                 const SkColor colors[], const SkScalar pos[], int count) {
        return CreateSweep(cx, cy, colors, pos, count, 0, NULL);
    }

#ifdef SK_SUPPORT_LEGACY_GRADIENT_FACTORIES
    static SkShader* CreateSweep(SkScalar cx, SkScalar cy,
                                 const SkColor colors[], const SkScalar pos[], int count,
                                 void* ignored,
                                 uint32_t flags, const SkMatrix* localMatrix) {
        return CreateSweep(cx, cy, colors, pos, count, flags, localMatrix);
    }
#endif

    SK_DECLARE_FLATTENABLE_REGISTRAR_GROUP()
};

#endif
