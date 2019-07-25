




































#ifndef MOZILLA_GFX_HELPERSSKIA_H_
#define MOZILLA_GFX_HELPERSSKIA_H_

#include "2D.h"
#include "skia/SkCanvas.h"

namespace mozilla {
namespace gfx {

static inline SkBitmap::Config
GfxFormatToSkiaConfig(SurfaceFormat format)
{
  switch (format)
  {
    case FORMAT_B8G8R8A8:
      return SkBitmap::kARGB_8888_Config;
    case FORMAT_B8G8R8X8:
      
      return SkBitmap::kARGB_8888_Config;
    case FORMAT_R5G6B5:
      return SkBitmap::kRGB_565_Config;
    case FORMAT_A8:
      return SkBitmap::kA8_Config;

  }

  return SkBitmap::kARGB_8888_Config;
}

static inline void
GfxMatrixToSkiaMatrix(const Matrix& mat, SkMatrix& retval)
{
    retval.setAll(SkFloatToScalar(mat._11), SkFloatToScalar(mat._21), SkFloatToScalar(mat._31),
                  SkFloatToScalar(mat._12), SkFloatToScalar(mat._22), SkFloatToScalar(mat._32),
                  0, 0, SK_Scalar1);
}

}
}

#endif 
