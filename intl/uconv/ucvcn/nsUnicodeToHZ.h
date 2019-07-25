




 







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
                            int32_t * aSrcLength, 
                            char * aDest, 
                            int32_t * aDestLength);

  NS_IMETHOD FinishNoBuff(char * aDest, int32_t * aDestLength);

  NS_IMETHOD ConvertNoBuffNoErr(const PRUnichar * aSrc, int32_t * aSrcLength, 
                                char * aDest, int32_t * aDestLength)
  {
    return NS_OK;
  }  

  uint16_t mHZState;
protected:
  nsGBKConvUtil mUtil;


};

#endif 
