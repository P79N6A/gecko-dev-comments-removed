






#ifndef __nsCSSColorUtils_h
#define __nsCSSColorUtils_h

#include "mozilla/MathAlgorithms.h"

#include "nsColor.h"




#define NS_SUFFICIENT_LUMINOSITY_DIFFERENCE 125000
#define NS_LUMINOSITY_DIFFERENCE(a, b) \
          mozilla::DeprecatedAbs(NS_GetLuminosity(a) - NS_GetLuminosity(b))


void NS_GetSpecial3DColors(nscolor aResult[2],
                           nscolor aBackgroundColor,
                           nscolor aBorderColor);


int NS_GetBrightness(uint8_t aRed, uint8_t aGreen, uint8_t aBlue);



int32_t NS_GetLuminosity(nscolor aColor);


void NS_RGB2HSV(nscolor aColor, uint16_t &aHue, uint16_t &aSat,
                uint16_t &aValue, uint8_t &aAlpha);


void NS_HSV2RGB(nscolor &aColor, uint16_t aHue, uint16_t aSat, uint16_t aValue,
                uint8_t aAlpha);

#endif
