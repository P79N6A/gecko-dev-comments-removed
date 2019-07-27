






#ifndef SkTestScalerContext_DEFINED
#define SkTestScalerContext_DEFINED

#include "SkPaint.h"
#include "SkPath.h"
#include "SkTDArray.h"
#include "SkTypeface.h"

SkTypeface* CreateTestTypeface(SkPaint::FontMetrics (*funct)(SkTDArray<SkPath*>& pathArray,
                               SkTDArray<SkFixed>& widthArray),
                               SkTypeface::Style style);

#endif
