




































#ifndef nsUnicodeToGB2312V2_h___
#define nsUnicodeToGB2312V2_h___

#include "nsUCSupport.h"
#include "gbku.h"










class nsUnicodeToGB2312V2 : public nsEncoderSupport
{
public:

  


  nsUnicodeToGB2312V2();

protected:

  NS_IMETHOD ConvertNoBuff(const PRUnichar * aSrc, 
                            PRInt32 * aSrcLength, 
                            char * aDest, 
                            PRInt32 * aDestLength);

  
  

  NS_IMETHOD ConvertNoBuffNoErr(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
                                char * aDest, PRInt32 * aDestLength)
  {
    return NS_OK;
  }   

  NS_IMETHOD FillInfo(PRUint32 *aInfo);
protected:
  nsGBKConvUtil mUtil;
};

#endif 
