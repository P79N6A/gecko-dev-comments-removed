






#ifndef SkPaintPriv_DEFINED
#define SkPaintPriv_DEFINED

class SkBitmap;
class SkPaint;

#include "SkTypes.h"








bool isPaintOpaque(const SkPaint* paint,
                   const SkBitmap* bmpReplacesShader = NULL);






bool NeedsDeepCopy(const SkPaint& paint);
#endif
