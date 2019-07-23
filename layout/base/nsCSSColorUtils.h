






































#ifndef __nsCSSColorUtils_h
#define __nsCSSColorUtils_h

#include "nsColor.h"




#define NS_SUFFICIENT_LUMINOSITY_DIFFERENCE 125000
#define NS_LUMINOSITY_DIFFERENCE(a, b) \
          PR_ABS(NS_GetLuminosity(a) - NS_GetLuminosity(b))




void NS_Get3DColors(nscolor aResult[2], nscolor aBackgroundColor);


void NS_GetSpecial3DColors(nscolor aResult[2],
                           nscolor aBackgroundColor,
                           nscolor aBorderColor);


int NS_GetBrightness(PRUint8 aRed, PRUint8 aGreen, PRUint8 aBlue);



PRInt32 NS_GetLuminosity(nscolor aColor);


void NS_RGB2HSV(nscolor aColor,PRUint16 &aHue,PRUint16 &aSat,PRUint16 &aValue);

void NS_HSV2RGB(nscolor &aColor,PRUint16 aHue,PRUint16 aSat,PRUint16 aValue);

#endif
