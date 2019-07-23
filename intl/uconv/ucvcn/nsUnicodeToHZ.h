




































 







#ifndef nsUnicodeToHZ_h___
#define nsUnicodeToHZ_h___

#include "nsUCSupport.h"
#include "gbku.h"



class nsUnicodeToHZ: public nsEncoderSupport
{
public:

  


  nsUnicodeToHZ();

protected:

  
  
  NS_IMETHOD ConvertNoBuff(const PRUnichar * aSrc, 
                            PRInt32 * aSrcLength, 
                            char * aDest, 
                            PRInt32 * aDestLength);

  NS_IMETHOD FinishNoBuff(char * aDest, PRInt32 * aDestLength);

  NS_IMETHOD FillInfo(PRUint32 *aInfo);

  NS_IMETHOD ConvertNoBuffNoErr(const PRUnichar * aSrc, PRInt32 * aSrcLength, 
                                char * aDest, PRInt32 * aDestLength)
  {
    return NS_OK;
  }  

  PRUint16 mHZState;
protected:
  nsGBKConvUtil mUtil;


};

#endif 
