




































#ifndef nsUnicodeToBIG5NoAscii_h___
#define nsUnicodeToBIG5NoAscii_h___

#include "nsUCSupport.h"








class nsUnicodeToBIG5NoAscii : public nsTableEncoderSupport
{
public:

  


  nsUnicodeToBIG5NoAscii();

  NS_IMETHOD FillInfo(PRUint32 *aInfo);
protected:
};

#endif
