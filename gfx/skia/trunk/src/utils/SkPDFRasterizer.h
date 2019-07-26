





#ifndef SkPDFRasterizer_DEFINED
#define SkPDFRasterizer_DEFINED

#include "SkBitmap.h"
#include "SkStream.h"

#ifdef SK_BUILD_POPPLER
bool SkPopplerRasterizePDF(SkStream* pdf, SkBitmap* output);
#endif  

#ifdef SK_BUILD_NATIVE_PDF_RENDERER
bool SkNativeRasterizePDF(SkStream* pdf, SkBitmap* output);
#endif  

#endif
