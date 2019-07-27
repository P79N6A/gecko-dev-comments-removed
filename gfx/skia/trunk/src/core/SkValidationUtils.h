






#ifndef SkValidationUtils_DEFINED
#define SkValidationUtils_DEFINED

#include "SkBitmap.h"
#include "SkXfermode.h"



static inline bool SkIsValidCoeff(SkXfermode::Coeff coeff) {
    return coeff >= 0 && coeff < SkXfermode::kCoeffCount;
}



static inline bool SkIsValidMode(SkXfermode::Mode mode) {
    return (mode >= 0) && (mode <= SkXfermode::kLastMode);
}



static inline bool SkIsValidIRect(const SkIRect& rect) {
    return rect.width() >= 0 && rect.height() >= 0;
}



static inline bool SkIsValidRect(const SkRect& rect) {
    return (rect.fLeft <= rect.fRight) &&
           (rect.fTop <= rect.fBottom) &&
           SkScalarIsFinite(rect.width()) &&
           SkScalarIsFinite(rect.height());
}

#endif
