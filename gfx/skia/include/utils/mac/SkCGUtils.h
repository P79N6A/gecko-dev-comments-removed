






#ifndef SkCGUtils_DEFINED
#define SkCGUtils_DEFINED

#include "SkTypes.h"

#ifdef SK_BUILD_FOR_MAC
#include <ApplicationServices/ApplicationServices.h>
#endif

#ifdef SK_BUILD_FOR_IOS
#include <CoreGraphics/CoreGraphics.h>
#endif

class SkBitmap;
class SkStream;





CGImageRef SkCreateCGImageRefWithColorspace(const SkBitmap& bm,
                                            CGColorSpaceRef space);





static inline CGImageRef SkCreateCGImageRef(const SkBitmap& bm) {
    return SkCreateCGImageRefWithColorspace(bm, NULL);
}







void SkCGDrawBitmap(CGContextRef, const SkBitmap&, float x, float y);

bool SkPDFDocumentToBitmap(SkStream* stream, SkBitmap* output);

#endif
